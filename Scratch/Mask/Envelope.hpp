/**
 * 包络版掩码提取
 */

#pragma once

#include "Masking.hpp"

class CMaskingEnvelope : public IMasking
{
public:
    virtual int process(const cv::Mat* image, cv::Mat* mask, const struct ScratchParameter* parameter = nullptr) override;
};