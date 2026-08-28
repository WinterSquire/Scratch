#include "Scratch.hpp"

#include <QJsonDocument>
#include <opencv2/opencv.hpp>

#include "Mask/Envelope.hpp"
#include "Mask/NoEnvelope.hpp"

#include "Contour/Gaussian.hpp"
#include "Contour/Skeleton.hpp"

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
    struct ScratchParameter& parameter, 
    struct ScratchResult& result,
    cv::Mat* debugImages[NumberOfScratchAnalyseStage])
{
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
        errorCode = masking->process(&image, &mask, parameter.masking.data, debugImages ? debugImages[ScratchAnalyseStageMasking] : NULL);
    
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

        auto& debugImage = *debugImages[ScratchAnalyseStageMasking];
        if (image.channels() == 1)
            cv::cvtColor(image, debugImage, cv::COLOR_GRAY2BGR);
        else if (image.channels() == 4)
            cv::cvtColor(image, debugImage, cv::COLOR_BGRA2BGR);
        else
            debugImage = image.clone();

        // mask 白色区域显示为蓝色，mask 黑色区域保留原图。
        debugImage.setTo(cv::Scalar(255, 0, 0), mask);

        *debugImages[ScratchAnalyseStageContouring] = debugImage.clone();
    } while (0);

    // process image contour
    {
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

inline static void calculateScratchResultKinetic(
    double timeElapsed,
    struct ScratchResultFrame& frameCurrent,
    struct ScratchResultFrame& framePrevious,
    struct ScratchResultFrame& frameFirst)
{
    frameCurrent.heal = (frameFirst.scratchArea.pixel - frameCurrent.scratchArea.pixel) / frameFirst.scratchArea.pixel;
    frameCurrent.speed.area = (framePrevious.scratchArea.um - frameCurrent.scratchArea.um) / timeElapsed;
    frameCurrent.speed.width = (framePrevious.width.avg - frameCurrent.width.avg) / timeElapsed;
}

EScratchQuality CScratchController::analyseScratch(
    const cv::Mat& image, 
    struct ScratchParameter& parameter, 
    struct ScratchResult& result,
    cv::Mat* debugImages[NumberOfScratchAnalyseStage])
{
    return ::analyseScratch(image, parameter, result, debugImages);
}

int CScratchController::analyseScratchKinetic(
    const cv::Mat* images,
    const uint64_t* timestamps,
    struct ScratchResultFrame* frames,
    size_t size,
    struct ScratchParameter& parameter,
    struct ScratchResultKinetic& result,
    cv::Mat* debugImages[NumberOfScratchAnalyseStage])
{
    int level = 0;
    double healList[NumberOfFrames];
    uint64_t timestampList[NumberOfFrames];
    std::vector<double> times(size), timesElapsed(size);

    timestampList[FrameCurrent] = timestampList[FramePrevious] = timestampList[FrameFirst] = timestamps[0];

    for (int i = 0; i < size; ++i)
    {
        timestampList[FramePrevious] = timestampList[FrameCurrent];
        timestampList[FrameCurrent] = timestamps[i];
        times[i] = (timestampList[FrameCurrent] - timestampList[FrameFirst]) / 3600.0;
        timesElapsed[i] = (timestampList[FrameCurrent] - timestampList[FramePrevious]) / 3600.0;
    }
    
    for (int i = 0; i < size; ++i)
    {
        frames[i].quality = ::analyseScratch(images[i], parameter, frames[i], debugImages);
        
        if (!debugImages)
            continue;
        ++debugImages[ScratchAnalyseStageMasking];
        ++debugImages[ScratchAnalyseStageContouring];
    }

    for (int i = 1; i < size; ++i)
        ::calculateScratchResultKinetic(timesElapsed[i], frames[i], frames[i-1], frames[0]);

    healList[FrameCurrent] = healList[FramePrevious] = healList[FrameFirst] = frames[0].heal;

    for (int i = 1; i < size; ++i)
    {
        if (level > 1)
            break;
        
        healList[FramePrevious] = healList[FrameCurrent];
        healList[FrameCurrent] = frames[i].heal;

        if (level == 0 && healList[FrameCurrent] >= 0.5)
        {
            ++level;
            result.t50 = times[i-1] + (timesElapsed[i] * (0.5 - healList[FramePrevious]) / (healList[FrameCurrent] - healList[FramePrevious]));
        }

        if (level == 1 && healList[FrameCurrent] >= 0.9)
        {
            ++level;
            result.t90 = times[i-1] + (timesElapsed[i] * (0.9 - healList[FramePrevious]) / (healList[FrameCurrent] - healList[FramePrevious]));
        }
    }

    return 0;
}

int CScratchController::analyseScratchKineticOnce(
    const cv::Mat& image,
    const uint64_t timestamps[NumberOfFrames],
    struct ScratchResultFrame* frames[NumberOfFrames],
    struct ScratchParameter& parameter,
    struct ScratchResultKinetic& result,
    cv::Mat* debugImages[NumberOfScratchAnalyseStage])
{
    auto timeElapsed = (timestamps[FrameCurrent] - timestamps[FramePrevious]) / 3600.0;

    frames[FrameCurrent]->quality = ::analyseScratch(image, parameter, *frames[FrameCurrent], debugImages);

    ::calculateScratchResultKinetic(timeElapsed, *frames[FrameCurrent], *frames[FramePrevious], *frames[FrameFirst]);

    auto heal = frames[FrameCurrent]->heal;

    if (result.t50 == 0 && heal >= 0.5)
    {
        auto healBase = frames[FramePrevious]->heal;
        auto timeBase = (timestamps[FramePrevious] - timestamps[FrameFirst]) / 3600.0;
        result.t50 = timeBase + (timeElapsed * (0.5 - healBase) / (heal - healBase));
    }

    if (result.t90 == 0 && heal >= 0.9)
    {
        auto healBase = frames[FramePrevious]->heal;
        auto timeBase = (timestamps[FramePrevious] - timestamps[FrameFirst]) / 3600.0;
        result.t50 = timeBase + (timeElapsed * (0.9 - healBase) / (heal - healBase));
    }

    return 0;
}
