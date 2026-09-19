#pragma once

#include "IPwmChannel.hpp"

#include <cstdint>
#include <filesystem>
#include <string>


namespace rpi::pwm
{

class HardwarePwmChannel final : public IPwmChannel
{
public:
    HardwarePwmChannel(unsigned int gpio, 
                       unsigned int hw_channel, 
                       std::uint32_t frequency = 1000, 
                       double duty_percent = 0.0, 
                       std::filesystem::path pwm_chip = "/sys/class/pwm/pwmchip0");

    ~HardwarePwmChannel() override;

    HardwarePwmChannel(const HardwarePwmChannel&) = delete;
    HardwarePwmChannel& operator=(const HardwarePwmChannel&) = delete;

    bool start() override;
    bool stop() noexcept override;

    bool setDutyCycle(double duty_percent) override;
    bool setFrequency(std::uint32_t frequency) override;

    double dutyCycle() const noexcept override { return duty_percent_; }
    std::uint32_t frequency() const noexcept override { return frequency_; }
    bool isRunning() const noexcept override { return running_; }

    unsigned int gpio() const noexcept { return gpio_; }
    unsigned int hwChannel() const noexcept { return hw_channel_; }


private:
    std::filesystem::path channelPath() const;
    bool ensureExported();
    bool writeAttribute(const std::string& name, const std::string& value) const noexcept;
    bool writeUInt64(const std::string& name, std::uint64_t value) const noexcept;
    void unexportIfOwned() noexcept;
    bool applyConfiguration();

    unsigned int gpio_;
    unsigned int hw_channel_;
    std::uint32_t frequency_;
    double duty_percent_;
    std::filesystem::path pwm_chip_;
    bool running_ = false;
    bool exported_by_us_ = false;
};
}  //// namespace rpi::pwm