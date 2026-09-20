#include "AdaptiveInfraredController.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "../../../test/debug.hpp"

namespace rpi::pwm
{

AdaptiveInfraredController::AdaptiveInfraredController(InfraredLampController &lamp_controller,
                                                       const WeightMatrix &weights,
                                                       const ChannelMap &channels)
    : AdaptiveInfraredController(lamp_controller, weights, channels, Config{})
{
}

AdaptiveInfraredController::AdaptiveInfraredController(InfraredLampController &lamp_controller,
                                                       const WeightMatrix &weights,
                                                       const ChannelMap &channels,
                                                       const Config &config)
    : lamp_controller_(lamp_controller),
        weights_(weights),
        channels_(channels),
        config_(config)
{
    if(config_.target_gray < 0.0 || config_.target_gray > 255.0)
    {
        throw std::invalid_argument("target_gray must be between 0 and 255.");
    }

    if (config_.deadband < 0.0)
    {
        throw std::invalid_argument("deadband must be >= 0.");
    }

    if (config_.kp < 0.0)
    {
        throw std::invalid_argument("kp must be >= 0.");
    }

    if (config_.min_brightness < 0.0 || config_.max_brightness > 100.0 ||
        config_.min_brightness > config_.max_brightness)
    {
        throw std::invalid_argument("Invalid brightness range.");
    }

    if (config_.filter_alpha <= 0.0 || config_.filter_alpha > 1.0)
    {
        throw std::invalid_argument("filter_alpha must be in (0, 1].");
    }

    if (config_.stable_frames_required == 0)
    {
        throw std::invalid_argument("stable_frames_required must be > 0.");
    }

    if (config_.initial_brightness < config_.min_brightness ||
        config_.initial_brightness > config_.max_brightness)
    {
        throw std::invalid_argument("initial_brightness is outside brightness range.");
    }

    // 初始化8路PWM为初始亮度
    brightness_.fill(config_.initial_brightness);
} 

/**
 * 启动8路红外灯，并将PWM设置为初始值。
 */
bool AdaptiveInfraredController::start()
{
    if(!lamp_controller_.start())
    {
        DEBUG_ERROR("Exposure", "Failed to start infrared lamp controller");
        return false;
    }

    // 初始化8路pwm
    for(std::size_t i = 0; i < LampCount; ++i)
    {
        if(!lamp_controller_.setBrightness(channels_[i], brightness_[i]))
        {
            DEBUG_ERROR("Exposure", "Failed to initialize lamp channel " << channels_[i]
                << " at " << brightness_[i] << "%");
            // 如果没有设置成功
            lamp_controller_.allOff();
            return false;
        }
    }

    // 重新清除稳定状态
    resetStability();

    return true;
}


void AdaptiveInfraredController::stop() noexcept
{
    lamp_controller_.stop();
}


/**
 * 输入当前4个ROI的平均灰度，
 * 更新8路PWM。
 */
bool AdaptiveInfraredController::update(const RoiValues& roi_means)
{
    /*
     * ------------------------------------------------------------
     * 1. 对4个ROI的灰度做低通滤波
     * ------------------------------------------------------------
     */
    if(!filter_initialized_)
    {
        // 没有初始化，也就是刚开始的状态
        filtered_roi_means_ = roi_means;
        filter_initialized_ = true;
    }
    else
    {
        for(std::size_t i = 0; i < RoiCount; ++i)
        {
            filtered_roi_means_[i] = config_.filter_alpha * roi_means[i] + (1.0 - config_.filter_alpha) * filtered_roi_means_[i];
        }
    }

    /*
     * ------------------------------------------------------------
     * 2. 计算4个ROI的误差
     *
     * error = target - measured
     *
     * error > 0 -> ROI太暗
     * error < 0 -> ROI太亮
     * ------------------------------------------------------------
     */

     for(std::size_t i = 0; i < RoiCount; ++i)
     {
        const double raw_error = config_.target_gray - filtered_roi_means_[i];

        errors_[i] = applyDeadband(raw_error);
     }

    /*
     * ------------------------------------------------------------
     * 3. 计算8路PWM增量
     *
     * ΔP_j = Kp * Σ Wi,j * ei
     *
     * 即：
     *
     * ΔP = Kp * W^T * e
     * ------------------------------------------------------------
     */
    LampValues pwm_delta{};

    for(std::size_t lamp = 0; lamp < LampCount; ++lamp)
    {
        double weighted_error = 0.0;

        for(std::size_t roi = 0; roi < RoiCount; ++roi)
        {
            weighted_error += weights_[roi][lamp] * errors_[roi];
        }
        pwm_delta[lamp] = config_.kp * weighted_error;
    }


     /*
     * ------------------------------------------------------------
     * 4. 更新8路PWM
     * ------------------------------------------------------------
     */
    bool pwm_is_stable = true;

    for (std::size_t lamp = 0; lamp < LampCount; ++lamp)
    {
        const double old_brightness = brightness_[lamp];

        const double new_brightness = clampBrightness(old_brightness + pwm_delta[lamp]);

        const double actual_change = new_brightness - old_brightness;

        brightness_[lamp] = new_brightness;

        if (std::abs(actual_change) > config_.pwm_change_threshold)
        {
            pwm_is_stable = false;
        }

        if (!lamp_controller_.setBrightness(channels_[lamp], new_brightness))
        {
            DEBUG_ERROR("Exposure", "Failed to update lamp channel " << channels_[lamp]
                << " to " << new_brightness << "%");
            /*
             * 这里不直接修改 brightness_ 回旧值，
             * 因为硬件状态可能已经发生部分更新。
             */
            return false;
        }
    }

    /*
     * ------------------------------------------------------------
     * 5. 判断4个ROI是否全部进入目标范围
     * ------------------------------------------------------------
     */
    bool roi_is_in_target = true;

    for (std::size_t roi = 0; roi < RoiCount; ++roi)
    {
        const double raw_error = config_.target_gray - filtered_roi_means_[roi];

        if (std::abs(raw_error) > config_.deadband)
        {
            roi_is_in_target = false;
            break;
        }
    }

    /*
     * ------------------------------------------------------------
     * 6. 连续稳定帧计数
     * ------------------------------------------------------------
     */
    if (roi_is_in_target && pwm_is_stable)
    {
        ++stable_frame_count_;
    }
    else
    {
        stable_frame_count_ = 0;
    }

    return isStable();
}

bool AdaptiveInfraredController::isStable() const noexcept
{
    return stable_frame_count_ >= config_.stable_frames_required;
}

void AdaptiveInfraredController::resetStability() noexcept
{
    stable_frame_count_ = 0;
    filter_initialized_ = false;

    filtered_roi_means_.fill(0.0);
    errors_.fill(0.0);
}

void AdaptiveInfraredController::setWeights(const WeightMatrix& weights)
{
    weights_ = weights;
}

const AdaptiveInfraredController::LampValues& AdaptiveInfraredController::brightness() const noexcept
{
    return brightness_;
}

const AdaptiveInfraredController::RoiValues& AdaptiveInfraredController::filteredRoiMeans() const noexcept
{
    return filtered_roi_means_;
}

const AdaptiveInfraredController::RoiValues& AdaptiveInfraredController::errors() const noexcept
{
    return errors_;
}

double AdaptiveInfraredController::applyDeadband(double error) const noexcept
{
    if (std::abs(error) <= config_.deadband)
    {
        return 0.0;
    }

    return error;
}

double AdaptiveInfraredController::clampBrightness(double brightness) const noexcept
{
    return std::clamp(brightness, config_.min_brightness, config_.max_brightness);
}



}
