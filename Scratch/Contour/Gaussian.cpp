#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "Gaussian.hpp"
#include "../Scratch.hpp"

static void fillNan1D(const cv::Mat &input, cv::Mat &output)
{
    CV_Assert(input.cols == 1 || input.rows == 1);
    const int length = static_cast<int>(input.total());
    output.create(input.size(), CV_64F);

    for (int index = 0; index < length; ++index)
        output.at<double>(index) = input.at<double>(index);

    int first = -1;
    for (int index = 0; index < length; ++index) {
        if (std::isfinite(output.at<double>(index))) {
            first = index;
            break;
        }
    }
    if (first < 0)
        return;

    for (int index = 0; index < first; ++index)
        output.at<double>(index) = output.at<double>(first);

    int previous = first;
    for (int index = first + 1; index < length; ++index) {
        if (!std::isfinite(output.at<double>(index)))
            continue;

        const double start = output.at<double>(previous);
        const double end = output.at<double>(index);
        for (int missing = previous + 1; missing < index; ++missing) {
            const double ratio = static_cast<double>(missing - previous) /
                                    static_cast<double>(index - previous);
            output.at<double>(missing) = start + ratio * (end - start);
        }
        previous = index;
    }
    for (int index = previous + 1; index < length; ++index)
        output.at<double>(index) = output.at<double>(previous);

}

static void getRowBoundaries(const cv::Mat &mask,
                                cv::Mat &left,
                                cv::Mat &right)
{
    left = cv::Mat(mask.rows, 1, CV_64F,
                    cv::Scalar(std::numeric_limits<double>::quiet_NaN()));
    right = cv::Mat(mask.rows, 1, CV_64F,
                    cv::Scalar(std::numeric_limits<double>::quiet_NaN()));

    for (int y = 0; y < mask.rows; ++y) {
        int first = -1;
        int last = -1;
        for (int x = 0; x < mask.cols; ++x) {
            if (mask.at<uchar>(y, x) != 0) {
                if (first < 0)
                    first = x;
                last = x;
            }
        }
        if (first >= 0) {
            left.at<double>(y) = static_cast<double>(first);
            right.at<double>(y) = static_cast<double>(last);
        }
    }
}

static void smoothBoundaries(cv::Mat &left,
                                cv::Mat &right,
                                double sigma = 8.0)
{
    cv::Mat leftFilled;
    cv::Mat rightFilled;
    fillNan1D(left, leftFilled);
    fillNan1D(right, rightFilled);

    if (sigma <= 0.0) {
        left = leftFilled;
        right = rightFilled;
        return;
    }

    const int radius = std::max(1, static_cast<int>(std::ceil(4.0 * sigma)));
    const int kernelSize = radius * 2 + 1;
    cv::GaussianBlur(leftFilled, left, cv::Size(1, kernelSize),
                        0.0, sigma, cv::BORDER_REPLICATE);
    cv::GaussianBlur(rightFilled, right, cv::Size(1, kernelSize),
                        0.0, sigma, cv::BORDER_REPLICATE);
}

static void buildCenterline(const cv::Mat &left,
                            const cv::Mat &right,
                            cv::Mat &center)
{
    CV_Assert(left.size() == right.size() && left.type() == CV_64F &&
                right.type() == CV_64F);
    center.create(left.size(), CV_64F);
    for (int index = 0; index < static_cast<int>(left.total()); ++index)
        center.at<double>(index) =
            (left.at<double>(index) + right.at<double>(index)) / 2.0;
}

static double calculateTortuosity(const cv::Mat& edge)
{
    CV_Assert(edge.cols == 1);
    CV_Assert(edge.type() == CV_64F);

    double arcLength = 0.0;

    int firstY = -1;
    int lastY = -1;

    double prevX = 0.0;
    int prevY = -1;

    for (int y = 0; y < edge.rows; ++y)
    {
        double x = edge.at<double>(y, 0);

        // 跳过无效点
        if (std::isnan(x) || !std::isfinite(x))
            continue;

        if (firstY < 0)
        {
            // 第一个有效点
            firstY = y;
            lastY = y;

            prevX = x;
            prevY = y;

            continue;
        }

        // 当前有效点
        double dx = x - prevX;
        double dy = static_cast<double>(y - prevY);

        arcLength += std::sqrt(dx * dx + dy * dy);

        prevX = x;
        prevY = y;

        lastY = y;
    }

    if (firstY < 0 || lastY <= firstY)
        return std::numeric_limits<double>::quiet_NaN();

    double projectionLength =
        static_cast<double>(lastY - firstY);

    return arcLength / projectionLength;
}

#define ASSERT(exp)
#define HALT(msg)

static void buildResult(const cv::Mat &left,
                        const cv::Mat &right,
                        ScratchResult &result)
{
    uint64_t size = left.total();
    double widthAVG, widthSTD, widthMED, widthSum = 0;
    std::vector<double> widthList(size);

    ASSERT(left.total() == right.total());

    for (int i = 0; i < size; ++i)
    {
        auto leftBoundary = left.at<double>(i);
        auto rightBoundary = right.at<double>(i);

        if (!std::isfinite(leftBoundary) || !std::isfinite(rightBoundary))
        {
            HALT("出现无限值！");
            continue;
        }

        widthList[i] = rightBoundary - leftBoundary;
        widthSum += widthList[i];
    }

    widthSTD = 0;
    widthAVG = widthSum / size;

    for (int i = 0; i < size; ++i)
        widthSTD += (widthList[i] - widthAVG) * (widthList[i] - widthAVG);

    widthSTD = std::sqrt(widthSTD / size);

    std::sort(widthList.begin(), widthList.end());

    if (size % 2 == 1)
        widthMED = widthList[size/2];
    else
        widthMED = (widthList[size/2-1] + widthList[size/2]) / 2;

    result.width.avg = widthAVG;
    result.width.med = widthMED;
    result.width.std = widthSTD;
}

int CContouringGaussian::process(const cv::Mat *mask, ScratchResult *result)
{
    cv::Mat left, right, center;

    if (mask == nullptr || mask->empty() || mask->channels() != 1 ||
        result == nullptr)
        return 1;

    getRowBoundaries(*mask, left, right);
    cv::Mat smoothedLeft = left;
    cv::Mat smoothedRight = right;
    smoothBoundaries(smoothedLeft, smoothedRight, 10.0);
    // buildCenterline(smoothedLeft, smoothedRight, center);
    buildResult(smoothedLeft, smoothedRight, *result);

    result->roughness.left = calculateTortuosity(left);
    result->roughness.right = calculateTortuosity(right);
    
    return 0;
}