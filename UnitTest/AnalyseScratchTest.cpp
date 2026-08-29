#include <fstream>
#include <cmath>
#include <algorithm>
#include <QDir>
#include <QRegularExpression>
#include <QDateTime>

#ifdef _WIN32
#include <Windows.h>
#endif

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

    }

    TEST_CASE("TestAnalyseScratchKinetic")
    {
        struct ScratchLab
        {
            ScratchLab(size_t size) : size(size), expImageList(size), expImageMaskList(size), expImageContourList(size), conImageList(size), timestamps(size), expFrames(size), conFrames(size) {
                memset(&this->exp, 0, sizeof(this->exp));
                memset(&this->parameter, 0, sizeof(this->parameter));
                
                parameter.masking.method = MaskingNoEnvelope;
                parameter.contouring.method = ContouringGaussian;
                parameter.dx = parameter.dy = 1.0;

                exp.p = 0.8;
                exp.images = expImageList.data();
                exp.timestamps = timestamps.data();
                exp.frames = expFrames.data();
                exp.debugImages[ScratchAnalyseStageMasking] = expImageMaskList.data();
                exp.debugImages[ScratchAnalyseStageContouring] = expImageContourList.data();

                con.p = 0.7;
                con.images = conImageList.data();
                con.frames = conFrames.data();
                con.timestamps = timestamps.data();
            }

            void generate()
            {
                for (int i = 0; i < size; ++i)
                {
                    auto& expFrame = exp.frames[i];
                    auto& conFrame = con.frames[i];

                    conFrame = expFrame;
                    conFrame.heal = std::max(0.0, expFrame.heal * 0.85);
                    conFrame.confluence = std::max(0.0, expFrame.confluence * 1.05);
                    conFrame.scratchArea.pixel = expFrame.scratchArea.pixel * 1.10;
                    conFrame.scratchArea.um = expFrame.scratchArea.um * 1.10;
                    conFrame.width.avg = expFrame.width.avg * 1.08;
                    conFrame.width.std = expFrame.width.std * 1.08;
                    conFrame.width.med = expFrame.width.med * 1.08;
                    conFrame.roughness.left = expFrame.roughness.left * 1.12;
                    conFrame.roughness.right = expFrame.roughness.right * 1.12;
                    conFrame.speed.width = expFrame.speed.width * 0.90;
                    conFrame.speed.area = expFrame.speed.area * 0.90;
                }
            }

            size_t size;
            ScratchParameterGlobal parameter;
            ScratchParameterKinetic exp, con;
            
            std::vector<cv::Mat> expImageList, expImageMaskList, expImageContourList, conImageList;
            std::vector<uint64_t> timestamps;
            std::vector<ScratchResult> expFrames, conFrames;
        };        

        int errorCode;
        QRegularExpression re(R"((\d{8}_\d{6}))");
        QDir expImageDir(IMAGE_PATH_PREFIX, "*.jpg", QDir::Name, QDir::Files);
        auto expImageInfoList = expImageDir.entryInfoList();
        ScratchLab lab(expImageInfoList.size());
        
        REQUIRE(lab.size >= 2);

        do {
            ZoneScopedN("ReadImages");

            for (int i = 0; i < lab.size; ++i)
            {
                const auto& expImageInfo = expImageInfoList[i];

                auto date = re.match(expImageInfo.baseName()).captured(1);

                lab.exp.images[i] = cv::imread(expImageInfo.absoluteFilePath().toStdString());
                lab.exp.timestamps[i] = QDateTime::fromString(date, "yyyyMMdd_HHmmss").toSecsSinceEpoch();
            }        
        } while (0);

        do {
            ZoneScopedN("ProcessImages");
            errorCode = CScratchController::analyseScratchKinetic(lab.exp, lab.parameter, lab.size);
        } while (0);

        lab.generate();
        
        {   
            ZoneScopedN("Serialization");

            std::ofstream html(IMAGE_PATH_PREFIX "/index.html");
            std::ofstream dataJs(IMAGE_PATH_PREFIX "/data.js");
            std::ofstream imageJs(IMAGE_PATH_PREFIX "/images.js");

            lab.exp.debugImages[ScratchAnalyseStageMasking] = lab.expImageMaskList.data();
            lab.exp.debugImages[ScratchAnalyseStageContouring] = lab.expImageContourList.data();

            Scratch::createHTMLTemplate(html);
            CDataJsSerializer().serialize(lab.size, &lab.exp, &lab.con, lab.parameter, dataJs);
            CImagesJsSerializer().serialize(lab.size, &lab.exp, NULL, lab.parameter, imageJs);
        }

        std::filesystem::path path = std::filesystem::absolute("Data/Input/S1/index.html");

#ifdef _WIN32
        ShellExecuteW(
            nullptr,
            L"open",
            path.c_str(),
            nullptr,
            nullptr,
            SW_SHOWNORMAL
        );
#endif
    }
}
