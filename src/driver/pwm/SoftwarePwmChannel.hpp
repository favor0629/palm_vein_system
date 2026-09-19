#pragma once

#include "GpioChip.hpp"
#include "IPwmChannel.hpp"

#include <cstdint>
#include <memory>
#include <utility>


namespace rpi::pwm {

class SoftwarePwmChannel final : public IPwmChannel {
public:
    SoftwarePwmChannel(std::shared_ptr<GpioChip> gpio_chip,
                       unsigned int gpio,
                       std::uint32_t frequency = 1000,
                       double duty_percent = 0.0);

    ~SoftwarePwmChannel() override;

    SoftwarePwmChannel(const SoftwarePwmChannel&) = delete;
    SoftwarePwmChannel& operator=(const SoftwarePwmChannel&) = delete;

    bool start() override;
    bool stop() noexcept override;

    bool setDutyCycle(double duty_percent) override;
    bool setFrequency(std::uint32_t frequency) override;

    double dutyCycle() const noexcept override { return duty_percent_; }
    std::uint32_t frequency() const noexcept override { return frequency_; }
    bool isRunning() const noexcept override { return running_; }

    unsigned int gpio() const noexcept { return gpio_; }

private:
    bool applyPwm();
    bool applyStaticLevel(int level);

    std::shared_ptr<GpioChip> gpio_chip_;
    unsigned int gpio_;
    std::uint32_t frequency_;
    double duty_percent_;
    bool running_ = false;
    bool claimed_ = false;
    bool pwm_active_ = false;
};

} // namespace rpi::pwm