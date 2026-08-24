#include "NoEnvelope.hpp"

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

int CMaskingNoEnvelope::process(const cv::Mat *image, cv::Mat *mask, const void *data)
{
    if (image == nullptr || mask == nullptr || image->empty() || image->channels() != 3)
        return 1;

    cv::Mat hsv;
    cv::cvtColor(*image, hsv, cv::COLOR_RGB2HSV);

    cv::Mat hue, saturation, value;
    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);
    hue = channels[0];
    saturation = channels[1];
    value = channels[2];

    cv::Mat selected = (hue >= 15) & (hue <= 40) &
                        (saturation >= 60) & (value >= 40);
    selected.convertTo(selected, CV_8UC1, 1.0 / 255.0);

    *mask = largestComponent(fillHoles(directionalFill(selected)));

    return 0;
}