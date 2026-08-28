#pragma once

#include "Contouring.hpp"

class CContouringGaussian : public IContouring
{
public:
    int process(const cv::Mat& mask, struct ScratchResult& result, const void* data = nullptr, cv::Mat* debugImage = nullptr) override;
};