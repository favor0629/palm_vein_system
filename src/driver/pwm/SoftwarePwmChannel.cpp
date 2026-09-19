#include "SoftwarePwmChannel.hpp"

#include <lgpio.h>


#include <stdexcept>
#include <string>




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
    if (!gpio_chip_) 
    {
        throw std::invalid_argument("SoftwarePwmChannel requires a valid GpioChip");
    }
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

    const int rc = lgGpioClaimOutput(gpio_chip_->handle(), 0, static_cast<int>(gpio_), LG_LOW);
    if (rc < 0) 
    {
        return false;
    }

    claimed_ = true;
    pwm_active_ = false;

    if (!applyPwm()) 
    {
        (void)lgGpioWrite(gpio_chip_->handle(), static_cast<int>(gpio_), LG_LOW);
        (void)lgGpioFree(gpio_chip_->handle(), static_cast<int>(gpio_));
        claimed_ = false;
        return false;
    }

    running_ = true;
    return true;
}

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

    success = lgGpioWrite(gpio_chip_->handle(), static_cast<int>(gpio_), LG_LOW) >= 0 && success;
    success = lgGpioFree(gpio_chip_->handle(), static_cast<int>(gpio_)) >= 0 && success;

    claimed_ = false;
    running_ = false;
    return success;
}

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

    const double old_duty = duty_percent_;
    duty_percent_ = duty_percent;
    if (!applyPwm()) 
    {
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
