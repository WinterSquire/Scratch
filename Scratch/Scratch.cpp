#include "Scratch.hpp"

#include <QJsonDocument>
#include <opencv2/opencv.hpp>

#include "Mask/Envelope.hpp"
#include "Mask/NoEnvelope.hpp"

#include "Contour/Gaussian.hpp"
#include "Contour/Skeleton.hpp"

#define DEBUG_IMAGE_BACKGROUND_COLOR cv::Scalar(0xd3, 0xd3, 0x05)

union MaskingStorage
{
    char envelope[sizeof(class CMaskingEnvelope)];
    char noEnvelope[sizeof(class CMaskingNoEnvelope)];
};

union ContouringStorage
{
    char gaussian[sizeof(class CContouringGaussian)];
    char skeleton[sizeof(class CContouringSkeleton)];
};

inline static EScratchQuality analyseScratch(
    const cv::Mat& image, 
    struct ScratchParameterGlobal& parameter, 
    struct ScratchResult& result,
    cv::Mat* debugImages[NumberOfScratchAnalyseStage])
{
    ZoneScoped;

    cv::Mat mask;
    int errorCode;
    EScratchQuality quality = ScratchQualityNormal;
    
    MaskingStorage maskingStorage{};
    ContouringStorage contouringStorage{};
    IMasking* masking = (IMasking*)&maskingStorage;
    IContouring* contouring = (IContouring*)&contouringStorage;

    // initinalize algorithm interface
    switch (parameter.contouring.method)
    {
        case ContouringGaussian: new (&contouringStorage) CContouringGaussian(); break;
        case ContouringSkeleton: new (&contouringStorage) CContouringSkeleton(); break;
        default: HALT("out of range");
    }

    switch (parameter.masking.method)
    {
        case MaskingNoEnvelope: new (&maskingStorage) CMaskingNoEnvelope(); break;
        case MaskingEnvelope: new (&maskingStorage) CMaskingEnvelope(); break;
        default: HALT("out of range");
    } 

    // process image mask
    do {
        ZoneScopedN("Masking");
        errorCode = masking->process(image, mask, parameter.masking.data, debugImages ? debugImages[ScratchAnalyseStageMasking] : NULL);
    
        if (errorCode)
            return ScratchQualityAbnormal;

        // 统计image和mask的像素数量，并计算汇合度
        result.scratchArea.pixel = static_cast<double>(cv::countNonZero(mask));
        result.scratchArea.um = result.scratchArea.pixel * parameter.dx * parameter.dy;

        result.invasionArea.um = 0;
        result.invasionArea.pixel = 0;

        const double totalPixels = static_cast<double>(mask.total());
        result.confluence = totalPixels > 0.0
            ? 1.0 - (result.scratchArea.pixel / totalPixels)
            : 0.0;

        // 绘制debug图像
        if (debugImages == NULL)
            break;

        // mask 白色区域显示为蓝色，mask 黑色区域保留原图。
        *debugImages[ScratchAnalyseStageMasking] = image.clone();
        debugImages[ScratchAnalyseStageMasking]->setTo(DEBUG_IMAGE_BACKGROUND_COLOR, mask);

        *debugImages[ScratchAnalyseStageContouring] = debugImages[ScratchAnalyseStageMasking]->clone();
    } while (0);

    // process image contour
    {
        ZoneScopedN("Contouring");
        errorCode = contouring->process(mask, result, parameter.contouring.data, debugImages ? debugImages[ScratchAnalyseStageContouring] : NULL);

        if (errorCode)
            return ScratchQualityAbnormal;

        result.width.avg *= parameter.dx;
        result.width.std *= parameter.dx;
        result.width.med *= parameter.dx;
    }

    if (std::round(result.width.std / result.width.avg * 100) > 25.0)
        quality = ScratchQualityUneven; // 划痕不均匀

    return quality;
}

inline static void calculateScratchResult(
    double timeElapsed,
    struct ScratchResult& frameCurrent,
    struct ScratchResult& framePrevious,
    struct ScratchResult& frameFirst)
{
    frameCurrent.heal = (frameFirst.scratchArea.pixel - frameCurrent.scratchArea.pixel) / frameFirst.scratchArea.pixel;
    if (timeElapsed <= 1e-6) return;
    frameCurrent.speed.area = (framePrevious.scratchArea.um - frameCurrent.scratchArea.um) / timeElapsed;
    frameCurrent.speed.width = (framePrevious.width.avg - frameCurrent.width.avg) / timeElapsed;
}

int CScratchController::analyseScratchKinetic(struct ScratchParameterKinetic& parameter, struct ScratchParameterGlobal& gParameter, size_t size)
{
    ZoneScoped;

    int level = 0;
    double healList[NumberOfFrames];
    uint64_t timestampList[NumberOfFrames];
    std::vector<double> times(size), timesElapsed(size);

    timestampList[FrameCurrent] = timestampList[FramePrevious] = timestampList[FrameFirst] = parameter.timestamps[0];

{
    ZoneScopedN("CalculateTimestamp");
    for (int i = 0; i < size; ++i)
    {
        timestampList[FramePrevious] = timestampList[FrameCurrent];
        timestampList[FrameCurrent] = parameter.timestamps[i];
        times[i] = (timestampList[FrameCurrent] - timestampList[FrameFirst]) / 3600.0;
        timesElapsed[i] = (timestampList[FrameCurrent] - timestampList[FramePrevious]) / 3600.0;
    }
}
   
    
    for (int i = 0; i < size; ++i)
    {
        parameter.frames[i].quality = analyseScratch(parameter.images[i], gParameter, parameter.frames[i], parameter.debugImages);
    
        if (parameter.debugImages[ScratchAnalyseStageMasking]) ++parameter.debugImages[ScratchAnalyseStageMasking];
        if (parameter.debugImages[ScratchAnalyseStageContouring]) ++parameter.debugImages[ScratchAnalyseStageContouring];
    }

{
    ZoneScopedN("CalculateScratchResult");
    for (int i = 1; i < size; ++i)
    {
        calculateScratchResult(timesElapsed[i], parameter.frames[i], parameter.frames[i-1], parameter.frames[0]);
    }
}

    healList[FrameCurrent] = healList[FramePrevious] = healList[FrameFirst] = parameter.frames[0].heal;

{
    ZoneScopedN("CalculateT50T90");
    for (int i = 1; i < size; ++i)
    {
        if (level > 1)
            break;
        
        healList[FramePrevious] = healList[FrameCurrent];
        healList[FrameCurrent] = parameter.frames[i].heal;

        if (level == 0 && healList[FrameCurrent] >= 0.5)
        {
            ++level;
            parameter.t50 = times[i-1] + (timesElapsed[i] * (0.5 - healList[FramePrevious]) / (healList[FrameCurrent] - healList[FramePrevious]));
        }

        if (level == 1 && healList[FrameCurrent] >= 0.9)
        {
            ++level;
            parameter.t90 = times[i-1] + (timesElapsed[i] * (0.9 - healList[FramePrevious]) / (healList[FrameCurrent] - healList[FramePrevious]));
        }
    }
}

    return 0;
}

int CScratchController::analyseScratchKineticOnce(struct ScratchParameterKineticOnce& parameter, struct ScratchParameterGlobal& gParameter)
{
    auto timeElapsed = (parameter.timestamps[FrameCurrent] - parameter.timestamps[FramePrevious]) / 3600.0;

    parameter.frames[FrameCurrent]->quality = ::analyseScratch(*parameter.image, gParameter, *parameter.frames[FrameCurrent], parameter.debugImages);

    ::calculateScratchResult(timeElapsed, *parameter.frames[FrameCurrent], *parameter.frames[FramePrevious], *parameter.frames[FrameFirst]);

    auto heal = parameter.frames[FrameCurrent]->heal;

    if (parameter.t50 == 0 && heal >= 0.5)
    {
        auto healBase = parameter.frames[FramePrevious]->heal;
        auto timeBase = (parameter.timestamps[FramePrevious] - parameter.timestamps[FrameFirst]) / 3600.0;
        parameter.t50 = timeBase + (timeElapsed * (0.5 - healBase) / (heal - healBase));
    }

    if (parameter.t90 == 0 && heal >= 0.9)
    {
        auto healBase = parameter.frames[FramePrevious]->heal;
        auto timeBase = (parameter.timestamps[FramePrevious] - parameter.timestamps[FrameFirst]) / 3600.0;
        parameter.t50 = timeBase + (timeElapsed * (0.9 - healBase) / (heal - healBase));
    }

    return 0;
}
