#include <fstream>
#include <cmath>
#include <QDir>
#include <QRegularExpression>
#include <QDateTime>

#include <Scratch.hpp>

#include "doctest.h"

#define IMAGE_PATH_PREFIX "Data/Input/S1"

#define CHECK_DOUBLE_ABS(a, b, abs_eps)                        \
    do {                                                       \
        auto&& _a = (a);                                       \
        auto&& _b = (b);                                       \
        auto&& _eps = (abs_eps);                               \
        CHECK(std::fabs(_a - _b) <= _eps);                     \
    } while(0)

TEST_SUITE("AnalyseScratchTest")
{
    TEST_CASE("TestAnalyseScratch")
    {
        ScratchParameter parameter{};
        ScratchResult result{};

        auto imageName = "A1_11_12_20250413_083750_cellScratch.jpg";
        auto imagePath = std::string() + IMAGE_PATH_PREFIX + '/' + imageName;
        auto imageRGB = cv::imread(imagePath, cv::IMREAD_COLOR_RGB);
        auto debugDirectory = QStringLiteral("Data/Output/S1");

        parameter.dx = parameter.dy = 1.0;        
        parameter.masking.method = MaskingNoEnvelope;
        parameter.contouring.method = ContouringGaussian;

        auto timeBegin = std::chrono::high_resolution_clock::now();
        auto imageQuality = CScratchController::analyseScratch(imageRGB, parameter, result);
        auto timeEnd = std::chrono::high_resolution_clock::now();
        auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeBegin).count();

        MESSAGE("Spend Time: " << timeElapsed << " ms");
        MESSAGE(("NoEnvelope + Guassian: \n" + toJsonString(result)));

        // parameter.contouring.method = ContouringSkeleton;
        // timeBegin = std::chrono::high_resolution_clock::now();
        // imageQuality = CScratchController::analyseScratch(imageRGB, parameter, result);
        // timeEnd = std::chrono::high_resolution_clock::now();
        // timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeBegin).count();

        // MESSAGE("Spend Time: " << timeElapsed << " ms");
        // MESSAGE(("NoEnvelope + Skeleton: \n" + toJsonString(result)));
    }

    TEST_CASE("TestAnalyseScratchKinetic")
    {
        int errorCode;
        ScratchParameter parameter{};
        ScratchResultKinetic result{};
        QRegularExpression re(R"((\d{8}_\d{6}))");
        QDir expImageDir(IMAGE_PATH_PREFIX, "*.jpg", QDir::Name, QDir::Files);
        auto expImageInfoList = expImageDir.entryInfoList();
        auto size = expImageInfoList.size();
        auto expImageList = std::vector<cv::Mat>(size);
        auto expImageMaskList = std::vector<cv::Mat>(size);
        auto expImageContourList = std::vector<cv::Mat>(size);
        auto timestampList = std::vector<uint64_t>(size);
        auto frames = std::vector<ScratchResultFrame>(size);

        cv::Mat* expDebugImages[NumberOfScratchAnalyseStage];

        expDebugImages[ScratchAnalyseStageMasking] = expImageMaskList.data();
        expDebugImages[ScratchAnalyseStageContouring] = expImageContourList.data();

        for (int i = 0; i < size; ++i)
        {
            const auto& expImageInfo = expImageInfoList[i];

            auto date = re.match(expImageInfo.baseName()).captured(1);

            expImageList[i] = cv::imread(expImageInfo.absoluteFilePath().toStdString(), cv::IMREAD_COLOR_RGB);
            timestampList[i] = QDateTime::fromString(date, "yyyyMMdd_HHmmss").toSecsSinceEpoch();
        }

        parameter.dx = parameter.dy = 1.0;

        REQUIRE(expImageList.size() == timestampList.size());
        REQUIRE(expImageList.size() >= 2);

        auto timeBegin = std::chrono::high_resolution_clock::now();
        errorCode = CScratchController::analyseScratchKinetic(
            expImageList.data(),
            timestampList.data(),
            frames.data(),
            size,
            parameter,
            result,
            expDebugImages);

        REQUIRE(errorCode == 0);
        auto timeEnd = std::chrono::high_resolution_clock::now();
        auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeBegin).count();

        MESSAGE("Spend Time: " << timeElapsed << " ms");
        
        {            
            std::ofstream jsonFile(IMAGE_PATH_PREFIX "/data.js");

            jsonFile << "const DATA = ";
            
            CJSONSerializer::process(
                timestampList.data(),
                frames.data(),
                size,
                result,
                jsonFile);
        }

        {
            std::ofstream jsonFile(IMAGE_PATH_PREFIX "/images.js");
            const cv::Mat* mats[2];
            
            mats[ScratchAnalyseStageMasking] = expImageMaskList.data();
            mats[ScratchAnalyseStageContouring] = expImageContourList.data();

            jsonFile << "const IMAGES = ";

            CJSONSerializer::process(
                expImageList.data(),
                mats,
                size,
                jsonFile);
        }
    }
}
