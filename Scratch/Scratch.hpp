/**
 * 划痕实验接口
 */
#pragma once

#include <cstdint>
#include <opencv2/opencv.hpp>

#include "Serialization/CSV.hpp"
#include "Serialization/HTML.hpp"
#include "Serialization/JSON.hpp"

#define HALT(msg) 

#define SetFlag32(flags, flag, b)  (flags = (b) ? ((flags) | (1 >> (flag))) : ((flags) & ~(1 >> (flag))))
#define GetFlag32(flags, flag)     (((flags) & (1 >> (flag))) != 0)

/* ---------- enums */

enum EScratchParameterFlag
{
    ScratchParameterFlagDrawDebugImage,
};

enum EScratchAnalyseStage
{
    ScratchAnalyseStageMasking,
    ScratchAnalyseStageContouring,
    NumberOfScratchAnalyseStage
};

enum EFrame
{
    FrameCurrent,
    FramePrevious,
    FrameFirst,
    NumberOfFrames
};

enum EMasking
{
    MaskingNoEnvelope,
    MaskingEnvelope,
};

enum EContouring
{
    ContouringGaussian,
    ContouringSkeleton,
};

/**
 * @brief 划痕质量枚举，标记单帧划痕分析结果可信程度
 */
enum EScratchQuality
{
    ScratchQualityNormal,   ///< 正常，划痕分割结果可信
    ScratchQualitySmall,    ///< 划痕过小
    ScratchQualityUneven,   ///< 划痕不均匀，左右前沿差异大，量化结果可信度下降
    ScratchQualityAbnormal  ///< 异常：未检测划痕/划痕面积过小/分割置信度低，结果仅供参考
};

/* ---------- 控制器类 */

class CScratchController
{
public:
    /**
     * @brief 划痕实验单张图像分析接口
     * 对输入明场/相差图像执行划痕分割、前沿提取与量化指标计算
     * @param[in] image 输入图像
     * @param[in] parameter 输入分析参数
     * @param[out] result 输出分析结果
     * @return 划痕质量枚举 EScratchQuality
     * @note 输入图像分割已完成；内部不做帧间配准，时序序列需上层完成配准后再逐帧调用
     */
    static enum EScratchQuality analyseScratch(
        const cv::Mat& image, 
        struct ScratchParameter& parameter, 
        struct ScratchResult& result);

    /**
     * @brief 划痕实验时序图像分析接口
     * 对输入明场/相差图像执行划痕分割、前沿提取与量化指标计算
     * @param[in] images 输入图像的数组指针，不能为NULL
     * @param[in] timestamps 输入图像时间戳的数组指针，单位：秒，不能为NULL
     * @param[out] frames 输出图像帧分析的数组指针，不能为NULL
     * @param[in] size 输入图像的个数
     * @param[in] parameter 输入分析参数
     * @param[out] result 输出分析结果
     * @return int 错误码，0代表处理成功；非0为异常错误码（图像为空、分割失败等）
     */
    static int analyseScratchKinetic(
        const cv::Mat* images,
        const uint64_t* timestamps,
        struct ScratchResultFrame* frames,
        size_t size,
        struct ScratchParameter& parameter,
        struct ScratchResultKinetic& result);

    /**
     * @brief 划痕实验时序图像单张分析接口
     * 对输入明场/相差图像执行划痕分割、前沿提取与量化指标计算
     * @param[in] image 输入图像
     * @param[in] timestamps 输入图像时间戳的数组指针，单位：秒，不能为NULL
     * @param[out] frames 输出图像帧分析的数组指针，不能为NULL
     * @param[in] parameter 输入分析参数
     * @param[out] result 输出分析结果
     * @return int 错误码，0代表处理成功；非0为异常错误码（图像为空、分割失败等）
     */
    static int analyseScratchKineticOnce(
        const cv::Mat& image,
        const uint64_t timestamps[NumberOfFrames],
        struct ScratchResultFrame* frames[NumberOfFrames],
        struct ScratchParameter& parameter,
        struct ScratchResultKinetic& result);
};

/* ---------- 数据结构体 */

typedef int Point2D[2];

struct MaksingEnvelopeParameter
{
    int kernelSize;
};

/**
 * @brief 划痕分析输入参数结构体
 */
struct ScratchParameter
{
    int flags;
    double dx, dy;                  ///< 单像素物理尺寸

    struct {
        int method;                 ///< 
        const void* data;           ///< 
    } masking, contouring;

    struct {
        Point2D* lines;             ///< 
        uint64_t size;              ///< 
    } partition;    
    
    cv::Mat debugImages[NumberOfScratchAnalyseStage];
};

/**
 * @brief 面积结构体
 */
struct ScratchArea
{
    double pixel;   ///< 像素面积，单位：像素²
    double um;      ///< 物理面积，单位：μm²
};

/**
 * @brief 划痕单帧分析输出结果结构体
 */
struct ScratchResult
{
    struct ScratchArea scratchArea;     ///< 划痕伤口区域面积
    struct ScratchArea invasionArea;    ///< 细胞侵入区域面积

    /**
     * @brief 划痕宽度统计
     */
    struct
    {
        double avg;     ///< 划痕平均宽度，单位 μm
        double std;     ///< 划痕宽度标准差，单位 μm，反映划痕宽窄均匀度
        double med;     ///< 划痕宽度中位数，单位 μm，反映划痕宽窄均匀度
    } width;

    /**
     * @brief 细胞迁移前沿粗糙度
     * @note 数值1.0代表前沿平直；大于1代表前沿锯齿、突起越明显
     */
    struct
    {
        double left;    ///< 左侧细胞前沿粗糙度
        double right;   ///< 右侧细胞前沿粗糙度
    } roughness;

    double confluence;  ///< 细胞汇合度，细胞占整个图像视野面积的百分比，取值范围 0.0 ~ 1.0
};

/**
 * @brief 划痕时序分析输出结果结构体
 */
struct ScratchResultFrame : ScratchResult
{
    double heal;                ///< 伤口愈合率 (%)

    struct {
        double width;           ///< 面积闭合速度，单位 μm/h
        double area;            ///< 宽度闭合速度，单位 μm²/h
    } speed;

    int quality;                ///< 质量校验
};

struct ScratchResultKinetic
{
    double t50, t90;    ///< 闭合率首次达到 50% / 90% 所需时间
};
