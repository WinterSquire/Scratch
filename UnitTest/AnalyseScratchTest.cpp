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

        parameter.dx = parameter.dy = 1.0;

        auto imageName = "A1_11_12_20260228_131443_cellScratch.jpg";
        auto imagePath = std::string() + IMAGE_PATH_PREFIX + '/' + imageName;
        auto imageRGB = cv::imread(imagePath, cv::IMREAD_COLOR_RGB);

        auto timeBegin = std::chrono::high_resolution_clock::now();
        auto imageQuality = CScratchController::analyseScratch(imageRGB, parameter, result);
        auto timeEnd = std::chrono::high_resolution_clock::now();
        auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeBegin).count();

        MESSAGE("Spend Time: " << timeElapsed << " ms");
        MESSAGE(toJsonString(result));

        CHECK_EQ(result.scratchArea.pixel, 2181514.00);
        CHECK_DOUBLE_ABS(result.width.avg, 1064.22, 1e-2);
        CHECK_DOUBLE_ABS(result.width.std, 15.15, 1e-4);
        CHECK_DOUBLE_ABS(result.roughness.left, 1.07, 1e-4);
        CHECK_DOUBLE_ABS(result.roughness.right, 1.02, 1e-4);
        CHECK_DOUBLE_ABS(result.confluence, 0.5649, 1e-4);
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
        auto timestampList = std::vector<uint64_t>(size);
        auto frames = std::vector<ScratchResultFrame>(size);

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
            result);

        REQUIRE(errorCode == 0);
        auto timeEnd = std::chrono::high_resolution_clock::now();
        auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeBegin).count();

        MESSAGE("Spend Time: " << timeElapsed << " ms");
        
        std::ofstream jsonFile(IMAGE_PATH_PREFIX "/Result.json");
        
        CJSONSerializer::process(
            timestampList.data(),
            frames.data(),
            size,
            result,
            jsonFile);

        std::ofstream csvFile(IMAGE_PATH_PREFIX "/Result.csv");

        CCSVSerializer::process(
            timestampList.data(),
            frames.data(),
            size,
            result,
            csvFile);
    }
}
