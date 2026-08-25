#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdint>
#include <ctime>
#include <regex>
#include <cmath>
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

namespace fs = std::filesystem;

inline bool parseTimestampFromFilename(const std::string& filename, uint64_t& out_ts)
{
    // 正则匹配 20260228_131443
    static const std::regex pat(R"((\d{8})_(\d{6}))");
    std::smatch m;
    if (!std::regex_search(filename, m, pat))
    {
        return false;
    }
    std::string dateStr = m[1].str();
    std::string timeStr = m[2].str();

    std::tm tm_obj{};
    // yyyyMMdd
    tm_obj.tm_year = std::stoi(dateStr.substr(0,4)) - 1900;
    tm_obj.tm_mon  = std::stoi(dateStr.substr(4,2)) - 1;
    tm_obj.tm_mday = std::stoi(dateStr.substr(6,2));

    // HHmmss
    tm_obj.tm_hour = std::stoi(timeStr.substr(0,2));
    tm_obj.tm_min  = std::stoi(timeStr.substr(2,2));
    tm_obj.tm_sec  = std::stoi(timeStr.substr(4,2));

    tm_obj.tm_isdst = -1; // 让mktime自动判断夏令时
    std::time_t sec = std::mktime(&tm_obj);
    if(sec == -1)
    {
        return false;
    }
    out_ts = static_cast<uint64_t>(sec);
    return true;
}

/// @brief 扫描目录，读取图片，同时解析时间戳；按时间戳升序排序
/// @param dirPath 目录 "Data/Input/S1"
/// @param outMats 输出图像列表
/// @param outTimestamps 输出unix时间戳(秒)，和图像一一对应
/// @return false遇到错误
inline bool scanScratchImageDir(
    const std::string& dirPath,
    std::vector<cv::Mat>& outMats,
    std::vector<uint64_t>& outTimestamps)
{
    outMats.clear();
    outTimestamps.clear();

    if(!fs::exists(dirPath) || !fs::is_directory(dirPath))
    {
        return false;
    }

    std::vector<std::pair<uint64_t, cv::Mat>> tempList;

    for(const auto& entry : fs::directory_iterator(dirPath))
    {
        if(!entry.is_regular_file()) continue;

        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](char c){ return static_cast<char>(std::tolower(c)); });

        // 图片后缀过滤
        if(ext != ".jpg" && ext != ".jpeg" && ext != ".png")
        {
            continue;
        }

        std::string filename = entry.path().filename().string();
        uint64_t ts{0};
        if(!parseTimestampFromFilename(filename, ts))
        {
            // 文件名无法解析时间戳，跳过该文件
            continue;
        }

        cv::Mat mat = cv::imread(entry.path().string(), cv::IMREAD_COLOR_RGB);
        if(mat.empty())
        {
            continue;
        }
        tempList.emplace_back(ts, std::move(mat));
    }

    // 按时间戳升序排序（时序实验必须时间从小到大）
    std::sort(tempList.begin(), tempList.end(),
        [](const auto& a, const auto& b){ return a.first < b.first; });

    for(auto&& item : tempList)
    {
        outTimestamps.push_back(item.first);
        outMats.push_back(std::move(item.second));
    }

    return !outMats.empty();
}

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
        auto imageQuality = CScratchController::analyseScratch(imageRGB, NULL, parameter, result);

        MESSAGE(toJsonString(result));

        CHECK_EQ(result.area.pixel, 2181514.00);
        CHECK_DOUBLE_ABS(result.width.avg, 1064.22, 1e-2);
        CHECK_DOUBLE_ABS(result.width.std, 15.15, 1e-4);
        CHECK_DOUBLE_ABS(result.roughness.left, 1.07, 1e-4);
        CHECK_DOUBLE_ABS(result.roughness.right, 1.02, 1e-4);
        CHECK_DOUBLE_ABS(result.confluence, 0.5649, 1e-4);
    }

    TEST_CASE("TestAnalyseScratchKinetic")
    {
        std::vector<cv::Mat> expImageList;
        std::vector<uint64_t> timestampList;

        // 扫描目录，自动读图片 + 提取文件名时间戳，自动按时间升序
        bool scanOk = scanScratchImageDir(IMAGE_PATH_PREFIX, expImageList, timestampList);
        REQUIRE(scanOk);
        REQUIRE(expImageList.size() == timestampList.size());
        REQUIRE(expImageList.size() >= 2); //时序分析至少2张

        size_t size = expImageList.size();

        ScratchParameter parameter{};
        parameter.dx = parameter.dy = 1.0;
        
        std::vector<ScratchResultFrame> frames(size);
        ScratchResultKinetic result{};
        ScratchInvasionData* invasionDataList = nullptr;

        result.frames = frames.data();

        // 对照组传 nullptr，不做增殖校正
        int retCode = CScratchController::analyseScratchKinetic(
            expImageList.data(),
            nullptr,
            timestampList.data(),
            size,
            parameter,
            result,
            invasionDataList);

        REQUIRE(retCode == 0);

        // 这里填入你的预期校验，示例占位
        // CHECK_DOUBLE_ABS(result.some_metric, expect_val, 1e‑2);

        // 可选：校验时间戳数组非递减
        for(size_t i = 1; i < timestampList.size(); ++i)
        {
            CHECK(timestampList[i] >= timestampList[i-1]);
        }

        std::ofstream file("TestAnalyseScratchKinetic.csv");

        CCSVSerializer::process(
            timestampList.data(),
            result,
            size,
            file);
    }
}
