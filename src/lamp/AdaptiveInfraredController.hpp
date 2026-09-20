#pragma once

#include "InfraredLampController.hpp"

#include <array>
#include <cstddef>



/**
 * 类的作用：根据摄像头看到的图像亮度，自动决定 8 路红外灯应该调到多亮
 * 系统里有：
    4 个 ROI（感兴趣区域）
    8 个红外灯
    摄像头不断提供 4 个 ROI 的平均灰度
    系统希望 4 个 ROI 的灰度都接近目标值，例如 120
    如果某个区域太暗，就增加相关红外灯亮度
    如果某个区域太亮，就降低相关红外灯亮度
    为了避免摄像头噪声导致灯光不停抖动，还要：
    先滤波
    设置死区
    判断 PWM 是否已经基本不变化
    连续稳定若干帧后，才认为真正稳定

    所以整个系统其实是一个闭环：
    摄像头
    ↓
    4个ROI灰度
    ↓
    AdaptiveInfraredController
    ↓
    计算8路PWM应该怎么变化
    ↓
    InfraredLampController
    ↓
    8路红外灯
    ↓
    照亮场景
    ↓
    摄像头再次采集
    ↓
    新的4个ROI灰度
    ↓
    再次调整
 */
namespace rpi::pwm
{

class AdaptiveInfraredController
{
public:
    static constexpr std::size_t RoiCount = 4;      // 暂时划分为4个视觉区域
    static constexpr std::size_t LampCount = 8;     // 8个红外灯

    using RoiValues = std::array<double, RoiCount>;
    /**
     * LampValues 是 8 个红外灯的亮度百分比，范围 [0, 100]。
     * 该数组的下标是逻辑灯号，而不是 GPIO 编号：
     */
    using LampValues = std::array<double, LampCount>;
    using WeightMatrix = std::array<std::array<double, LampCount>, RoiCount>;
    /**
     * ChannelMap 是 8 个红外灯对应的 PwmController 内部通道 ID。
     * 8 个“逻辑红外灯”，分别对应 InfraredLampController 的哪个 PWM Channel
     */
    using ChannelMap = std::array<unsigned int, LampCount>;

    struct Config
    {
        /* data */
        // 目标ROI平均灰度
        double target_gray = 120.0;

        // 死区，低于这个值怎么不进行调整
        double deadband = 5.0;

        // 比例控制系数
        double kp = 0.2;

        // PWM范围
        double min_brightness = 0.0;
        double max_brightness = 100.0;

        // ROI 灰度低通滤波系数
        // 1.0 = 不滤波，值越小越平滑
        double filter_alpha = 0.3;

        // 一个PWM更新小于该值是，可以认为基本无变化
        double pwm_change_threshold = 0.2;

        // 连续多少帧满足稳定条件进入稳定状态
        std::size_t stable_frames_required = 10;

        // 初始pwm
        double initial_brightness = 30.0;
    };

public:
    AdaptiveInfraredController(InfraredLampController &lamp_controller,
                               const WeightMatrix &weights,
                               const ChannelMap &channels);

    AdaptiveInfraredController(InfraredLampController &lamp_controller,
                               const WeightMatrix &weights,
                               const ChannelMap &channels,
                               const Config &config);
                        
    /**
     * 启动8路红外灯，并将PWM设置为初始值。
     */
    bool start();

    /**
     * 停止8路红外灯。
     */
    void stop() noexcept;

    /**
     * 输入当前4个ROI的平均灰度，
     * 更新8路PWM。
     *
     * 返回值：
     * true  -> 已经达到稳定状态
     * false -> 仍然处于自动调光阶段
     */
    bool update(const RoiValues& roi_means);

    /**
     * 是否已经稳定。
     */
    bool isStable() const noexcept;

    /**
     * 重新清除稳定状态。
     */
    void resetStability() noexcept;

    /**
     * 设置新的权重矩阵。
     */
    void setWeights(const WeightMatrix& weights);

    /**
     * 获取当前8路PWM。
     */
    const LampValues& brightness() const noexcept;

    /**
     * 获取最近一次滤波后的ROI灰度。
     */
    const RoiValues& filteredRoiMeans() const noexcept;

    /**
     * 获取最近一次误差。
     */
    const RoiValues& errors() const noexcept;

private:
    double applyDeadband(double error) const noexcept;

    double clampBrightness(double brightness) const noexcept;

private:
    InfraredLampController &lamp_controller_;    //真正负责操作8路红外灯的底层控制器

    WeightMatrix weights_;  //权重矩阵，行是ROI，列是灯。
    ChannelMap channels_;   // 逻辑灯编号 → PWM Channel编号
    Config config_;     //自动调光算法参数

    LampValues brightness_{};   //控制器当前记录的8路PWM

    RoiValues filtered_roi_means_{};    //低通滤波后的4个ROI平均灰度
    RoiValues errors_{};                //4个ROI的误差 = 目标灰度 - 当前灰度

    bool filter_initialized_{false};

    std::size_t stable_frame_count_{0};
};
}