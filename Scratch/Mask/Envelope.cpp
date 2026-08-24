#include "Envelope.hpp"

#include <opencv2/opencv.hpp>

#define MASKING_ENVELOPE_KERNEL_SIZE 41

int CMaskingEnvelope::process(const cv::Mat *image, cv::Mat *mask, const void *data)
{
    struct MaksingEnvelopeParameter defaultParameter;
    const struct MaksingEnvelopeParameter* parameter = (struct MaksingEnvelopeParameter *)data;

    if (image == nullptr || mask == nullptr || image->empty() || image->channels() != 3)
        return 1;

    if (parameter == NULL)
    {
        defaultParameter.kernelSize = MASKING_ENVELOPE_KERNEL_SIZE;
        parameter = &defaultParameter;
    }

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

    const cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_ELLIPSE, cv::Size(parameter->kernelSize, parameter->kernelSize));

    cv::Mat closed;
    cv::morphologyEx(selected, closed, cv::MORPH_CLOSE, kernel);

    *mask = largestComponent(fillHoles(directionalFill(closed)));

    return 0;
}
