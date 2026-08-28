#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/geometry/2d.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <queue>
#include <unordered_map>
#include <vector>

#include "Skeleton.hpp"
#include "../Scratch.hpp"

namespace
{
// 八邻域顺序为东、东南、南、西南、西、西北、北、东北；
// 该顺序同时用于计算骨架像素环绕时的 0->1 连通性变化次数。
constexpr int kNeighbourCount = 8;
const int kDx[kNeighbourCount] = {1, 1, 0, -1, -1, -1, 0, 1};
const int kDy[kNeighbourCount] = {0, 1, 1, 1, 0, -1, -1, -1};

int key(const cv::Point &point, int width)
{
    // 将二维像素坐标压成一维索引，便于保存 Dijkstra 的父节点。
    return point.y * width + point.x;
}

cv::Mat thin(const cv::Mat &source)
{
    // 纯 OpenCV 实现的 Zhang-Suen 细化。每轮分两个子步骤，
    // 先收集再统一删除，避免同一子步骤中删除顺序影响结果。
    cv::Mat image;
    source.convertTo(image, CV_8UC1);
    image = image > 0;

    bool changed = true;
    while (changed) {
        changed = false;
        for (int pass = 0; pass < 2; ++pass) {
            std::vector<cv::Point> remove;
            for (int y = 1; y < image.rows - 1; ++y) {
                for (int x = 1; x < image.cols - 1; ++x) {
                    if (!image.at<uchar>(y, x))
                        continue;
                    int neighbours = 0;
                    int transitions = 0;
                    for (int i = 0; i < kNeighbourCount; ++i) {
                        const bool current = image.at<uchar>(y + kDy[i], x + kDx[i]) != 0;
                        const bool next = image.at<uchar>(
                            y + kDy[(i + 1) % kNeighbourCount],
                            x + kDx[(i + 1) % kNeighbourCount]) != 0;
                        neighbours += current ? 1 : 0;
                        transitions += !current && next ? 1 : 0;
                    }
                    if (neighbours < 2 || neighbours > 6 || transitions != 1)
                        continue;
                    const bool north = image.at<uchar>(y - 1, x) != 0;
                    const bool east = image.at<uchar>(y, x + 1) != 0;
                    const bool south = image.at<uchar>(y + 1, x) != 0;
                    const bool west = image.at<uchar>(y, x - 1) != 0;
                    // 两个子步骤的方向约束用于保护局部连通性和端点。
                    if (pass == 0 ? (north && east && south) || (east && south && west)
                                  : (north && east && west) || (north && south && west))
                        continue;
                    remove.emplace_back(x, y);
                }
            }
            for (const cv::Point &point : remove)
                image.at<uchar>(point) = 0;
            changed = changed || !remove.empty();
        }
    }
    return image;
}

std::vector<cv::Point> skeletonPath(const cv::Mat &skeleton, const cv::Mat &distance)
{
    // 通过 PCA 得到划痕主轴，再从主轴投影最小/最大的骨架端点之间找主干。
    // 路径分数是沿途距离场的最小值，因此优先选择更居中的骨架路径。
    std::vector<cv::Point> points;
    cv::findNonZero(skeleton, points);
    if (points.size() < 2)
        return points;

    cv::Mat data(static_cast<int>(points.size()), 2, CV_64F);
    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
        data.at<double>(i, 0) = points[i].x;
        data.at<double>(i, 1) = points[i].y;
    }
    cv::PCA pca(data, cv::Mat(), cv::PCA::DATA_AS_ROW);
    const cv::Vec2d axis(pca.eigenvectors.at<double>(0, 0),
                         pca.eigenvectors.at<double>(0, 1));
    const cv::Vec2d center(pca.mean.at<double>(0, 0), pca.mean.at<double>(0, 1));

    auto neighbours = [&](const cv::Point &point) {
        std::vector<cv::Point> result;
        for (int i = 0; i < kNeighbourCount; ++i) {
            cv::Point next(point.x + kDx[i], point.y + kDy[i]);
            if (next.x >= 0 && next.x < skeleton.cols && next.y >= 0 &&
                next.y < skeleton.rows && skeleton.at<uchar>(next))
                result.push_back(next);
        }
        return result;
    };

    auto projection = [&](const cv::Point &point) {
        return (point.x - center[0]) * axis[0] + (point.y - center[1]) * axis[1];
    };
    const auto minProjection = std::min_element(
        points.begin(), points.end(), [&](const cv::Point &a, const cv::Point &b) {
            return projection(a) < projection(b);
        });
    const auto maxProjection = std::max_element(
        points.begin(), points.end(), [&](const cv::Point &a, const cv::Point &b) {
            return projection(a) < projection(b);
        });
    const double projectionMin = projection(*minProjection);
    const double projectionMax = projection(*maxProjection);
    const double cap = std::max(3.0, 0.05 * (projectionMax - projectionMin));

    auto selectEnd = [&](bool highEnd) {
        std::vector<cv::Point> candidates;
        for (const cv::Point &point : points) {
            const double value = projection(point);
            if (highEnd ? value >= projectionMax - cap : value <= projectionMin + cap)
                candidates.push_back(point);
        }

        std::vector<cv::Point> clipped;
        for (const cv::Point &point : candidates) {
            if (point.x == 0 || point.x == skeleton.cols - 1 || point.y == 0 ||
                point.y == skeleton.rows - 1)
                clipped.push_back(point);
        }
        if (!clipped.empty())
            candidates.swap(clipped);

        return *std::max_element(candidates.begin(), candidates.end(),
                                 [&](const cv::Point &a, const cv::Point &b) {
                                     return distance.at<float>(a) < distance.at<float>(b);
                                 });
    };
    const cv::Point start = selectEnd(false);
    const cv::Point finish = selectEnd(true);

    const int total = skeleton.rows * skeleton.cols;
    const float infinity = -std::numeric_limits<float>::infinity();
    std::vector<float> score(total, infinity);
    std::vector<int> parent(total, -1);
    using Item = std::pair<float, cv::Point>;

    struct CompareItem
    {
        bool operator()(const Item& left, const Item& right) const
        {
            return left.first < right.first;
        }
    };

    std::priority_queue<Item, std::vector<Item>, CompareItem> queue;
    score[key(start, skeleton.cols)] = distance.at<float>(start);
    queue.emplace(score[key(start, skeleton.cols)], start);

    // 最大瓶颈路径的 Dijkstra：score[p] 表示从 start 到 p 的最大最小距离。
    while (!queue.empty()) {
        const auto [currentScore, point] = queue.top();
        queue.pop();
        if (currentScore < score[key(point, skeleton.cols)] - 1e-6f)
            continue;
        if (point == finish)
            break;
        for (const cv::Point &next : neighbours(point)) {
            const float candidate = std::min(currentScore, distance.at<float>(next));
            const int nextKey = key(next, skeleton.cols);
            if (candidate > score[nextKey]) {
                score[nextKey] = candidate;
                parent[nextKey] = key(point, skeleton.cols);
                queue.emplace(candidate, next);
            }
        }
    }
    if (parent[key(finish, skeleton.cols)] < 0 && finish != start)
        return std::vector<cv::Point>();

    // 根据父节点表从终点回溯出有序中心线。
    std::vector<cv::Point> path;
    int current = key(finish, skeleton.cols);
    while (current >= 0) {
        path.emplace_back(current % skeleton.cols, current / skeleton.cols);
        current = parent[current];
    }
    std::reverse(path.begin(), path.end());
    return path;
}

} // namespace

// 返回 8 位单通道中心线图：中心线像素为 255，其余为 0。
// 该引用在下一次 process 或对象析构前有效。
// 返回值约定：0 表示处理完成（包括空掩膜），1 表示参数或输入格式错误。
int CContouringSkeleton::process(const cv::Mat& mask, struct ScratchResult& result, const void* data, cv::Mat* debugImage)
{
    cv::Mat centerline_;
    if (mask.empty() || mask.channels() != 1)
        return 1;

    cv::Mat binary;
    cv::compare(mask, 0, binary, cv::CMP_GT);

    // 距离变换给出每个前景像素到边界的距离，中轴处的 2 * distance 即局部宽度。
    cv::Mat distance;

    cv::distanceTransform(binary, distance, cv::DIST_L2, 5, CV_32F);
    centerline_ = thin(binary);
    const std::vector<cv::Point> path = skeletonPath(centerline_, distance);
    if (path.empty())
        return 0;

    std::vector<double> widths;
    std::vector<double> arc(path.size(), 0.0);
    for (size_t i = 1; i < path.size(); ++i)
        arc[i] = arc[i - 1] + cv::norm(path[i] - path[i - 1]);
    const double length = arc.back();
    // 沿有序路径累计弧长，每 20 像素取一个样本，避免骨架像素密度
    // 或斜线方向差异改变宽度统计的权重。
    if (length == 0.0) {
        widths.push_back(2.0 * distance.at<float>(path.front()));
    } else {
        const int count = std::max(8, static_cast<int>(std::ceil(length / 20.0)) + 1);
        for (int i = 0; i < count; ++i) {
            const double target = length * i / (count - 1);
            const auto upper = std::lower_bound(arc.begin(), arc.end(), target);
            const size_t index = static_cast<size_t>(std::distance(arc.begin(), upper));
            widths.push_back(2.0 * distance.at<float>(path[std::min(index, path.size() - 1)]));
        }
    }
    // 与 Python 版 s_eff 保持一致：端部或窄腰宽度低于最大宽度 80% 时不参与统计。
    const double maximumWidth = *std::max_element(widths.begin(), widths.end());
    std::vector<double> effectiveWidths;
    for (double width : widths) {
        if (width >= 0.8 * maximumWidth)
            effectiveWidths.push_back(width);
    }
    if (effectiveWidths.size() < 3)
        effectiveWidths = widths;

    // 这里按设计文档将宽度写入像素单位；若需要物理单位，调用方应乘 umPerPixel。
    const double sum = std::accumulate(effectiveWidths.begin(), effectiveWidths.end(), 0.0);

    result.width.avg = sum / effectiveWidths.size();
    double variance = 0.0;
    for (double width : effectiveWidths)
        variance += (width - result.width.avg) * (width - result.width.avg);
    result.width.std = std::sqrt(variance / effectiveWidths.size());
    std::sort(effectiveWidths.begin(), effectiveWidths.end());
    const size_t middle = effectiveWidths.size() / 2;
    result.width.med = effectiveWidths.size() % 2 != 0
        ? effectiveWidths[middle]
        : (effectiveWidths[middle - 1] + effectiveWidths[middle]) / 2.0;

    if (debugImage != NULL)
    {
        // 在二值掩膜上叠加最终主干，红色像素便于直接检查路径选择结果。
        for (const cv::Point &point : path)
            debugImage->at<cv::Vec3b>(point) = cv::Vec3b(0, 0, 255);
    }

    return 0;
}