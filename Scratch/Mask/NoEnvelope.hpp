/**
 * 无包络版掩码提取
 */

#pragma once

#include "Masking.hpp"

class CMaskingNoEnvelope : public IMasking
{
public:
    virtual int process(const cv::Mat* image, cv::Mat* mask, const void* data = nullptr) override;
};