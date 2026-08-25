#include "Scratch.hpp"

#include <QJsonDocument>
#include <opencv2/opencv.hpp>

#include "Mask/Envelope.hpp"
#include "Mask/NoEnvelope.hpp"

#include "Contour/Gaussian.hpp"

union MaskingStorage
{
    char envelope[sizeof(class CMaskingEnvelope)];
    char noEnvelope[sizeof(class CMaskingNoEnvelope)];
};

union ContouringStorage
{
    char gaussian[sizeof(class CContouringGaussian)];
};

EScratchQuality CScratchController::analyseScratch(
    const cv::Mat& expImage, 
    const cv::Mat* conImage, 
    const ScratchParameter& parameter, 
    ScratchResult& result, 
    ScratchInvasionData *invasionData)
{
    cv::Mat mask;
    int errorCode;
    EScratchQuality quality = ScratchQualityNormal;
    // 初始化算法接口
    MaskingStorage maskingStorage{};
    ContouringStorage contouringStorage{};
    IMasking* masking = (IMasking*)&maskingStorage;
    IContouring* contouring = (IContouring*)&contouringStorage;

    // 第一版：使用非包络法和高斯插值法
    {
        new (&maskingStorage) CMaskingNoEnvelope();
        new (&contouringStorage) CContouringGaussian();
    }    

    errorCode = masking->process(&expImage, &mask);
    
    if (errorCode)
        return ScratchQualityAbnormal;

    // 统计image和mask的像素数量，并计算汇合度
    {
        result.area.pixel = static_cast<double>(cv::countNonZero(mask));
        result.area.um = result.area.pixel * parameter.dx * parameter.dy;

        const double totalPixels = static_cast<double>(mask.total());
        result.confluence = totalPixels > 0.0
            ? 1.0 - (result.area.pixel / totalPixels)
            : 0.0;
    }

    errorCode = contouring->process(&mask, &result);

    if (errorCode)
        return ScratchQualityAbnormal;

    result.width.avg *= parameter.dx;
    result.width.std *= parameter.dx;
    result.width.med *= parameter.dx;

    if (std::round(result.width.std / result.width.avg * 100) > 25.0)
        quality = ScratchQualityUneven; // 划痕不均匀

    return quality;
}

int CScratchController::analyseScratchKinetic(
    const cv::Mat* expImageList, 
    const cv::Mat* conImageList, 
    const uint64_t* timestampList, 
    size_t size, 
    const ScratchParameter& parameter, 
    ScratchResultKinetic& result, 
    ScratchInvasionData* invasionDataList)
{
    int level = 0;

    for (int i = 0; i < size; ++i)
    {
        auto& resultKinetic = result.frames[i];
        auto& expImage = expImageList[i];

        resultKinetic.quality = analyseScratch(
            expImage, 
            NULL,
            parameter,
            resultKinetic.raw
        );
    }

    auto& fResult = result.frames[0];

    for (int i = 1; i < size; ++i)
    {
        auto& cResult = result.frames[i];
        auto& pResult = result.frames[i-1];
        auto timeElapsed = (timestampList[i] - timestampList[i-1]) / 3600.0;
        auto healRaw = (fResult.raw.area.pixel - cResult.raw.area.pixel) / fResult.raw.area.pixel;

        cResult.heal.raw = cResult.heal.corrected = healRaw;
        cResult.speed.area = (pResult.raw.area.um - cResult.raw.area.um) / timeElapsed;
        cResult.speed.width = (pResult.raw.width.avg - cResult.raw.width.avg) / timeElapsed;

        switch (level)
        {
        case 0:
            if (healRaw < 0.5)
                break;

            if (std::fabs(healRaw - 0.5) < 1e-4)
                result.t50 = (timestampList[i] - timestampList[0]) / 3600.0;
            else
                result.t50 = ((timestampList[i-1] - timestampList[0]) / 3600.0) + (timeElapsed * (0.5 - pResult.heal.corrected) / (cResult.heal.corrected - pResult.heal.corrected));

            ++level;
            break;
        case 1:
            if (healRaw < 0.9)
                break;

            if (std::fabs(healRaw - 0.9) < 1e-4)
                result.t90 = (timestampList[i] - timestampList[0]) / 3600.0;
            else
                result.t90 = ((timestampList[i-1] - timestampList[0]) / 3600.0) + (timeElapsed * (0.9 - pResult.heal.corrected) / (cResult.heal.corrected - pResult.heal.corrected));

            ++level;
            break;
        default:
            break;
        }
    }

    return 0;
}
