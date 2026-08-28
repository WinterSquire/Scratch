#pragma once

#include <opencv2/core.hpp>
#include "Contouring.hpp"

class CContouringSkeleton : public IContouring
{
public:
    // 使用距离变换和骨架主干计算平均宽度与宽度标准差。
    int process(const cv::Mat& mask, struct ScratchResult& result, const void* data = nullptr, cv::Mat* debugImage = nullptr) override;
};