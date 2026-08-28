#pragma once

namespace cv
{
    class Mat;
}

class IContouring
{
public:
    virtual int process(const cv::Mat* mask, struct ScratchResult* result,  struct ScratchParameter* parameter = nullptr) = 0;
};