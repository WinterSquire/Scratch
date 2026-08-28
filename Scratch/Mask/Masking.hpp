/**
 * 掩码提取算法接口定义
 */

#pragma once

namespace cv {
    class Mat;
}

class IMasking
{
public:
    /**
     * @brief 对彩色图像处理，提取出图像黑白掩码
     * @param[in] image 输入图像指针
     * @param[out] mask 输出图像掩码
     * @param[in] data 额外的数据，可选
     * @return 返回错误码，0为成功
     */
    virtual int process(const cv::Mat* image, cv::Mat* mask, const void* data = nullptr, cv::Mat* debugImage = nullptr) = 0;
};

/*---------- prototypes */

cv::Mat directionalFill(const cv::Mat &source);
cv::Mat fillHoles(const cv::Mat &source);
cv::Mat largestComponent(const cv::Mat &source);
