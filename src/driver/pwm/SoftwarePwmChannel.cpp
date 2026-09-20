#include "SoftwarePwmChannel.hpp"

#include <lgpio.h>


#include <stdexcept>
#include <string>

#include "../../../test/debug.hpp"




namespace rpi::pwm 
{

SoftwarePwmChannel::SoftwarePwmChannel(std::shared_ptr<GpioChip> gpio_chip,
                                       unsigned int gpio,
                                       std::uint32_t frequency,
                                       double duty_percent)
    : gpio_chip_(std::move(gpio_chip)),
      gpio_(gpio),
      frequency_(frequency),
      duty_percent_(duty_percent)
{
    /**
     * 对象根本没有办法正常工作。
     */
    if (!gpio_chip_) 
    {
        DEBUG_ERROR("PWM", "SoftwarePwmChannel received a null GpioChip");
        throw std::invalid_argument("SoftwarePwmChannel requires a valid GpioChip");
    }
    /**
     * PWM 频率必须大于 0 Hz，PWM 占空比必须在 [0, 100] 范围内。
     */
    validateFrequency(frequency_);
    validateDuty(duty_percent_);
}

SoftwarePwmChannel::~SoftwarePwmChannel()
{
    stop();
}

bool SoftwarePwmChannel::start()
{
    if (running_) 
    {
        return true;
    }

    // 向底层 GPIO 系统申请 GPIO，并配置为输出模式，初始电平为 LOW
    const int rc = lgGpioClaimOutput(gpio_chip_->handle(), 0, static_cast<int>(gpio_), LG_LOW);
    if (rc < 0) 
    {
        DEBUG_ERROR("PWM", "Failed to claim GPIO " << gpio_ << ", ret=" << rc);
        return false;
    }

    claimed_ = true;
    pwm_active_ = false;

    if (!applyPwm()) 
    {
        DEBUG_ERROR("PWM", "Failed to apply initial PWM on GPIO " << gpio_);
        (void)lgGpioWrite(gpio_chip_->handle(), static_cast<int>(gpio_), LG_LOW);
        (void)lgGpioFree(gpio_chip_->handle(), static_cast<int>(gpio_));
        claimed_ = false;
        return false;
    }

    running_ = true;    //只有操作真正成功之后，才能更新对象状态
    return true;
}

/**
 * 停止 PWM 输出。
 * noexcept 保证即使在 lgTxPwm() 或 lgGpioFree() 失败时也不会抛出异常。
 * 失败时返回 false，但仍会尝试释放 GPIO。
 * 该函数可在析构函数中调用，因此必须保证 noexcept。
 * 该函数可在 start() 失败时调用，因此必须保证 noexcept。
 */
bool SoftwarePwmChannel::stop() noexcept
{
    if (!claimed_) 
    {
        running_ = false;
        pwm_active_ = false;
        return true;
    }

    bool success = true;
    if (pwm_active_) 
    {
        success = lgTxPwm(
            gpio_chip_->handle(),
            static_cast<int>(gpio_),
            0.0F,
            0.0F,
            0,
            0) >= 0;
        pwm_active_ = false;
    }

    // success 累计所有操作的结果，确保即使某个操作失败也会尝试释放 GPIO
    success = lgGpioWrite(gpio_chip_->handle(), static_cast<int>(gpio_), LG_LOW) >= 0 && success;
    success = lgGpioFree(gpio_chip_->handle(), static_cast<int>(gpio_)) >= 0 && success;

    claimed_ = false;
    running_ = false;
    return success;
}

/**
 * 应用静态电平。
 */
bool SoftwarePwmChannel::applyStaticLevel(int level)
{
    if (pwm_active_) 
    {
        const int rc_stop = lgTxPwm(
            gpio_chip_->handle(),
            static_cast<int>(gpio_),
            0.0F,
            0.0F,
            0,
            0);
        if (rc_stop < 0) 
        {
            return false;
        }
        pwm_active_ = false;
    }

    return lgGpioWrite(
               gpio_chip_->handle(),
               static_cast<int>(gpio_),
               level) >= 0;
}

bool SoftwarePwmChannel::applyPwm()
{
    if (duty_percent_ <= 0.0) 
    {
        return applyStaticLevel(LG_LOW);
    }

    if (duty_percent_ >= 100.0) 
    {
        return applyStaticLevel(LG_HIGH);
    }

    // lgTxPwm provides software timed PWM. frequency=0 is documented as off,
    // duty is expressed as a percentage, and cycles=0 means continuous output.
    const int rc = lgTxPwm(
        gpio_chip_->handle(),
        static_cast<int>(gpio_),
        static_cast<float>(frequency_),
        static_cast<float>(duty_percent_),
        0,
        0);

    if (rc < 0) 
    {
        return false;
    }

    pwm_active_ = true;
    return true;
}

bool SoftwarePwmChannel::setDutyCycle(double duty_percent)
{
    try 
    {
        validateDuty(duty_percent);
    } 
    catch (const std::invalid_argument&) 
    {
        return false;
    }

    if (!running_) 
    {
        duty_percent_ = duty_percent;
        return true;
    }

    /**
     * 保存旧值
        ↓
        修改新值
        ↓
        尝试应用到硬件
        ↓
        成功？
        ┌─┴─┐
        是   否
        │     │
        ▼     ▼
        保留  恢复旧值
     */
    const double old_duty = duty_percent_;
    duty_percent_ = duty_percent;
    if (!applyPwm()) 
    {
        DEBUG_ERROR("PWM", "Failed to apply duty cycle " << duty_percent
            << "% on GPIO " << gpio_ << "; restoring previous value");
        duty_percent_ = old_duty;
        (void)applyPwm();
        return false;
    }
    return true;
}

bool SoftwarePwmChannel::setFrequency(std::uint32_t frequency)
{
    try 
    {
        validateFrequency(frequency);
    } 
    catch (const std::invalid_argument&) 
    {
        return false;
    }

    if (!running_) 
    {
        frequency_ = frequency;
        return true;
    }

    const std::uint32_t old_frequency = frequency_;
    frequency_ = frequency;
    if (!applyPwm()) 
    {
        frequency_ = old_frequency;
        (void)applyPwm();
        return false;
    }
    return true;
}

} // namespace rpi::pwm
