#include "Masking.hpp"

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#else
#define ZoneScoped
#define ZoneScopedN(name)
#endif

static double median(std::vector<double> values)
{
    if (values.empty())
        return std::numeric_limits<double>::quiet_NaN();
    const auto middle = values.begin() + values.size() / 2;
    std::nth_element(values.begin(), middle, values.end());
    if (values.size() % 2 != 0)
        return *middle;
    const double upper = *middle;
    const auto lower = std::max_element(values.begin(), middle);
    return (*lower + upper) / 2.0;
}

static void medianFilter(std::vector<double> &values, int size)
{
    const int radius = size / 2;
    std::vector<double> filtered(values.size());
    for (int index = 0; index < static_cast<int>(values.size()); ++index) {
        std::vector<double> window;
        for (int offset = -radius; offset <= radius; ++offset) {
            const int source = std::clamp(index + offset, 0,
                                            static_cast<int>(values.size()) - 1);
            window.push_back(values[source]);
        }
        filtered[index] = median(window);
    }
    values.swap(filtered);
}

/*---------- code */

/**
 * 方向性修复（directional fill）：
 *
 * 作用：
 *   该函数的目标是在二值掩码中，沿着目标主体的主轴方向修复“内部缺口/断裂”，
 *   让连通结构更完整、轮廓更连续。它不是简单的泛洪填充，而是基于对象方向和
 *   截面边界做有针对性的修复，因此更适合处理长条状、椭圆状或明显方向性的目标。
 *
 * 怎么做：
 *   1. 先把输入转成单通道二值图，保留最大连通域，去掉杂散噪声。
 *   2. 对前景像素做 PCA，估计主体的主方向 u 和法向方向 v，得到中心和长轴方向。
 *   3. 沿主轴把前景像素分桶，并统计每个截面在法向上的左右边界 left/right。
 *   4. 对缺失截面做线性插值，并用中值滤波平滑边界，构造稳定的“轮廓曲线”。
 *   5. 遍历整幅图像，判断哪些背景像素位于轮廓附近、属于可修复缺口区域，并标记
 *      inside / bg / bgForSearch / sideSeeds。
 *   6. 通过 connectedComponents 判断这些背景缺口是否可从侧边连通到外部背景；
 *      只修复“内部且不可达”的缺口，避免错误填充真实背景。
 *   7. 生成 repaired，并返回修复后的 Mask。
 *
 * 性能瓶颈：
 *   - 该函数会对整幅图像做多次遍历（至少 3~5 次全图扫描），并且每次都访问
 *     width * height 个像素，图像越大越慢。
 *   - 其后还执行 connectedComponents(bgForSearch, ...)，这是全图连通域分析，
 *     代价明显高于单纯的像素判断。
 *   - 额外创建了多个整图大小的 cv::Mat（bgForSearch、inside、bg、sideSeeds、labels），
 *     会产生较高的内存带宽和分配成本。
 *   - 在每个像素都要计算 s、n、bin、边界判定，且涉及 Vec2d、clamp、floor 等
 *     数学运算，常数开销不小。
 *   - 还有一些 vector 反复构造/重新分配（如 medianFilter、窗口采样、左右边界数组），
 *     在大图或高分辨率数据上也会放大系统开销。
 *
 * 结论：
 *   这个函数的本质不是“低复杂度的局部运算”，而是“全图方向分析 + 全图修复判定 +
 *   全图连通域分析”的组合。它的时间主要消耗在重复扫描整张图像和 cv::connectedComponents
 *   上，所以在大尺寸图上会明显变慢。
 */
cv::Mat directionalFill(const cv::Mat &source)
{
    ZoneScoped;
    // 1) 先把输入转成单通道二值图，并过滤掉像素过少的无效对象。
    //    之后只保留最大连通域，避免后续方向估计受到杂散噪声干扰。
    cv::Mat binary;
    source.convertTo(binary, CV_8UC1);
    if (cv::countNonZero(binary) < 16)
        return binary;

    cv::Mat main = largestComponent(binary);
    std::vector<cv::Point> points;
    cv::findNonZero(main, points);
    if (points.size() < 16)
        return binary;

    // 2) 用 PCA 估计目标主体的主方向 u 和法向方向 v。
    //    这里把所有前景像素视为一组二维点，主轴方向即“沿着目标延伸”的轴向。
    cv::Mat pcaData(static_cast<int>(points.size()), 2, CV_64F);
    for (int index = 0; index < static_cast<int>(points.size()); ++index) {
        pcaData.at<double>(index, 0) = points[index].x;
        pcaData.at<double>(index, 1) = points[index].y;
    }
    cv::PCA pca(pcaData, cv::Mat(), cv::PCA::DATA_AS_ROW);
    cv::Vec2d center(pca.mean.at<double>(0, 0), pca.mean.at<double>(0, 1));
    cv::Vec2d u(pca.eigenvectors.at<double>(0, 0),
                pca.eigenvectors.at<double>(0, 1));
    const double norm = std::sqrt(u.dot(u));
    if (norm <= std::numeric_limits<double>::epsilon())
        return binary;
    u /= norm;
    const cv::Vec2d v(-u[1], u[0]);

    const int height = binary.rows;
    const int width = binary.cols;
    std::vector<double> sMain;
    sMain.reserve(points.size());
    std::vector<double> nMain;
    nMain.reserve(points.size());
    for (const cv::Point &point : points) {
        const cv::Vec2d delta(point.x - center[0], point.y - center[1]);
        sMain.push_back(delta.dot(u));
        nMain.push_back(delta.dot(v));
    }

    // 3) 把目标沿主方向分桶（bin）并在每个桶内记录前景体在法向上的左右边界。
    //    left[i] / right[i] 表示第 i 个截面上，目标在法向方向上的最左/最右位置。
    const double s0 = *std::min_element(sMain.begin(), sMain.end());
    std::vector<int> bins(points.size());
    int binCount = 0;
    for (int index = 0; index < static_cast<int>(points.size()); ++index) {
        bins[index] = static_cast<int>(std::floor(sMain[index] - s0));
        binCount = std::max(binCount, bins[index] + 1);
    }
    if (binCount < 8)
        return binary;

    const double infinity = std::numeric_limits<double>::infinity();
    std::vector<double> left(binCount, infinity);
    std::vector<double> right(binCount, -infinity);
    for (int index = 0; index < static_cast<int>(points.size()); ++index) {
        left[bins[index]] = std::min(left[bins[index]], nMain[index]);
        right[bins[index]] = std::max(right[bins[index]], nMain[index]);
    }

    std::vector<int> valid;
    for (int index = 0; index < binCount; ++index) {
        if (std::isfinite(left[index]) && std::isfinite(right[index]))
            valid.push_back(index);
    }
    if (valid.size() < static_cast<size_t>(std::max(4, binCount / 4)))
        return binary;

    // 4) 对缺失的有效截面做插值，并执行轻度中值平滑，构造稳定的边界曲线。
    //    这样可以避免个别稀疏桶导致边界跳变，保证后续“填洞/修复”逻辑可靠。
    for (int index = 0; index < valid.front(); ++index) {
        left[index] = left[valid.front()];
        right[index] = right[valid.front()];
    }
    for (size_t position = 0; position + 1 < valid.size(); ++position) {
        const int first = valid[position];
        const int second = valid[position + 1];
        for (int index = first + 1; index < second; ++index) {
            const double ratio = static_cast<double>(index - first) / (second - first);
            left[index] = left[first] + ratio * (left[second] - left[first]);
            right[index] = right[first] + ratio * (right[second] - right[first]);
        }
    }
    for (int index = valid.back() + 1; index < binCount; ++index) {
        left[index] = left[valid.back()];
        right[index] = right[valid.back()];
    }

    const int smoothSize = std::max(3, static_cast<int>(std::round(binCount / 120.0)) * 2 + 1);
    if (smoothSize >= 5) {
        medianFilter(left, smoothSize);
        medianFilter(right, smoothSize);
    }
    for (int index = 0; index < binCount; ++index) {
        if (right[index] <= left[index])
            return binary;
    }

    // 5) 估计目标横向宽度，确保该结构确实是“厚实且可修复”的区域，而不是噪声或细线。
    const double width0 = median([&]() {
        std::vector<double> widths(binCount);
        for (int index = 0; index < binCount; ++index)
            widths[index] = right[index] - left[index];
        return widths;
    }());
    if (!std::isfinite(width0) || width0 < 4.0)
        return binary;

    const double sideMargin = std::max(2.0, std::min(8.0, 0.04 * width0));
    // bgForSearch: 需要搜索的背景候选区；inside: 已经位于主体内部的像素；bg: 需要保留的背景区域。
    cv::Mat bgForSearch = cv::Mat::zeros(binary.size(), CV_8UC1);
    cv::Mat inside = cv::Mat::zeros(binary.size(), CV_8UC1);
    cv::Mat bg = cv::Mat::zeros(binary.size(), CV_8UC1);
    const int cap = std::max(2, static_cast<int>(std::round(0.015 * binCount)));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const cv::Vec2d delta(x - center[0], y - center[1]);
            const double s = delta.dot(u);
            const double n = delta.dot(v);
            const int bin = std::clamp(static_cast<int>(std::floor(s - s0)), 0, binCount - 1);
            const bool in = n >= left[bin] && n <= right[bin];
            const bool work = n >= left[bin] - sideMargin && n <= right[bin] + sideMargin;
            const bool isBackground = binary.at<uchar>(y, x) == 0;
            const bool endpoint = bin <= cap || bin >= binCount - 1 - cap;
            if (in) inside.at<uchar>(y, x) = 1;
            if (isBackground && work) bg.at<uchar>(y, x) = 1;
            if (isBackground && work && !endpoint) bgForSearch.at<uchar>(y, x) = 1;
        }
    }

    // 6) 从两侧边缘找出“背景种子”，并根据连通性判断哪些缺口属于目标内部穿透区。
    //    只有与侧边相连、且在主体轮廓附近的背景才会被当作可修复的缺口；其余区域保持原状。
    cv::Mat sideSeeds = cv::Mat::zeros(binary.size(), CV_8UC1);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const cv::Vec2d delta(x - center[0], y - center[1]);
            const double s = delta.dot(u);
            const double n = delta.dot(v);
            const int bin = std::clamp(static_cast<int>(std::floor(s - s0)), 0, binCount - 1);
            const bool side = (n >= left[bin] - sideMargin && n < left[bin]) ||
                                (n > right[bin] && n <= right[bin] + sideMargin);
            if (side && bgForSearch.at<uchar>(y, x))
                sideSeeds.at<uchar>(y, x) = 1;
        }
    }
    if (cv::countNonZero(sideSeeds) == 0)
        return fillHoles(main);

    cv::Mat labels;
    cv::connectedComponents(bgForSearch, labels, 8, CV_32S);
    std::vector<bool> reachable(static_cast<size_t>(cv::countNonZero(labels == labels)), false);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (sideSeeds.at<uchar>(y, x))
                reachable[static_cast<size_t>(labels.at<int>(y, x))] = true;
        }
    }

    // 7) 最终把“内部且与背景相连但不属于可达外部区域”的缺口重建为前景，
    //    形成更完整、连续的主体掩码。
    cv::Mat repaired = main.clone();
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int label = labels.at<int>(y, x);
            if (inside.at<uchar>(y, x) && bg.at<uchar>(y, x) &&
                !reachable[static_cast<size_t>(label)])
                repaired.at<uchar>(y, x) = 1;
        }
    }
    return repaired;
}

cv::Mat fillHoles(const cv::Mat &source)
{
    ZoneScoped;
    cv::Mat flood;
    source.convertTo(flood, CV_8UC1, -255.0, 255.0);
    for (int x = 0; x < flood.cols; ++x) {
        if (flood.at<uchar>(0, x) == 255) cv::floodFill(flood, cv::Point(x, 0), 0);
        if (flood.at<uchar>(flood.rows - 1, x) == 255) cv::floodFill(flood, cv::Point(x, flood.rows - 1), 0);
    }
    for (int y = 0; y < flood.rows; ++y) {
        if (flood.at<uchar>(y, 0) == 255) cv::floodFill(flood, cv::Point(0, y), 0);
        if (flood.at<uchar>(y, flood.cols - 1) == 255) cv::floodFill(flood, cv::Point(flood.cols - 1, y), 0);
    }
    return source | (flood == 255);
}

cv::Mat largestComponent(const cv::Mat &source)
{
    ZoneScoped;
    cv::Mat binary;
    source.convertTo(binary, CV_8UC1);
    cv::Mat labels, statistics, centroids;
    const int count = cv::connectedComponentsWithStats(
        binary, labels, statistics, centroids, 8, CV_32S);
    if (count <= 1)
        return cv::Mat::zeros(source.size(), CV_8UC1);

    int largest = 1;
    for (int label = 2; label < count; ++label) {
        if (statistics.at<int>(label, cv::CC_STAT_AREA) >
            statistics.at<int>(largest, cv::CC_STAT_AREA))
            largest = label;
    }
    return labels == largest;
}