/**
 * 划痕实验接口
 */
#pragma once

#include <cstdint>
#include <QJsonObject>
#include <opencv2/opencv.hpp>

/* ---------- 控制器类 */

class CScratchController
{
public:
    /**
     * @brief 划痕实验单张图像分析接口
     * 对输入明场/相差图像执行划痕分割、前沿提取与量化指标计算
     * @param[in] expImage 实验组输入图像
     * @param[in] conImage 对照组输入图像，可为NULL
     * @param[in] parameter 输入分析参数
     * @param[out] result 输出分析结果
     * @param[out] invasionData 输出划痕区细胞侵入结果，可为NULL
     * @return 划痕质量枚举 EScratchQuality
     * @note 输入图像分割已完成；内部不做帧间配准，时序序列需上层完成配准后再逐帧调用
     */
    static enum EScratchQuality analyseScratch(
        const cv::Mat& expImage, 
        const cv::Mat* conImage, 
        const struct ScratchParameter& parameter,
        struct ScratchResult& result,
        struct ScratchInvasionData* invasionData = NULL);

    /**
     * @brief 划痕实验时序图像分析接口
     * 对输入明场/相差图像执行划痕分割、前沿提取与量化指标计算
     * @param[in] expImageList 输入实验组图像的数组指针，不能为NULL
     * @param[in] conImageList 输入对照组图像的数组指针，用于增殖校正，可为NULL
     * @param[in] timestampList 输入图像时间戳的数组指针，单位：秒，不能为NULL
     * @param[in] size 输入图像的个数
     * @param[in] parameter 输入分析参数
     * @param[out] result 输出分析结果
     * @param[out] invasionDataList 输出划痕区细胞侵入结果，数组指针，可为NULL
     * @return int 错误码，0代表处理成功；非0为异常错误码（图像为空、分割失败等）
     * @note 如果传入conImageList则会进行增值校正计算
     */
    static int analyseScratchKinetic(
        const cv::Mat* expImageList,
        const cv::Mat* conImageList,
        const uint64_t* timestampList,
        size_t size,
        const struct ScratchParameter& parameter,
        struct ScratchResultKinetic& result,
        struct ScratchInvasionData* invasionDataList = NULL);
};

/* ---------- 序列化接口 */
#define toJsonString(data) (QJsonDocument(toJson(data)).toJson(QJsonDocument::Compact))
#define fromJsonString(data, json) fromJson(data, (QJsonDocument::fromJson(jsonUtf8)));

QJsonObject toJson(const struct ScratchParameter& data);
int fromJson(struct ScratchParameter& data, const QJsonObject& json);

QJsonObject toJson(const struct ScratchArea& data);
int fromJson(struct ScratchArea& data, const QJsonObject& json);

QJsonObject toJson(const struct ScratchInvasionData& data);
int fromJson(struct ScratchInvasionData& data, const QJsonObject& json);

QJsonObject toJson(const struct ScratchResult& data);
int fromJson(struct ScratchResult& data, const QJsonObject& json);

QJsonObject toJson(const struct ScratchResultFrame& data);
int fromJson(struct ScratchResultFrame& data, const QJsonObject& json);

/* ---------- 数据结构体 */

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

/**
 * @brief 划痕分析输入参数结构体
 */
struct ScratchParameter
{
    int fillHole;       ///< 是否填充内部孔洞
    double dx, dy;      ///< 单像素物理尺寸
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
 * @brief 细胞侵袭数据
 */
struct ScratchInvasionData
{
    struct ScratchArea area;    ///< 划痕ROI框内细胞面接
    double ratio;               ///< 划痕框内细胞占比 [0‑1]；=cell_pixel / scratch_roi_total_pixel
};

/**
 * @brief 划痕单帧分析输出结果结构体
 */
struct ScratchResult
{
    struct ScratchArea area;    ///< 划痕伤口区域面积

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
struct ScratchResultFrame
{
    struct ScratchResult raw;   ///< 原始数据

    /**
     * @brief 伤口愈合百分比
     * @note 范围0‑100(%)；基于T0初始划痕面积计算原始愈合率
     */
    struct {
        double raw;             ///< 原始愈合率
        double corrected;       ///< 校正后愈合率，只有传入实验组时序图才有效
    } heal;

    struct {
        double width;           ///< 面积闭合速度，单位 μm/h
        double area;            ///< 宽度闭合速度，单位 μm²/h
    } speed;

    int quality;                ///< 质量校验
};

struct ScratchResultKinetic
{
    struct ScratchResultFrame* frames;  ///< 时序数据
    double t50, t90;                    ///< 闭合率首次达到 50% / 90% 所需时间
};
