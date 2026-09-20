#pragma once

#include "GpioChip.hpp"
#include "IPwmChannel.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace rpi::pwm {

class PwmController {
public:
    explicit PwmController(unsigned int gpio_chip = 0U);

    /**
     * PwmController 自己没有需要手动释放的 C 风格资源
     */
    ~PwmController() = default;

    PwmController(const PwmController&) = delete;
    PwmController& operator=(const PwmController&) = delete;

    bool addChannel(unsigned int id, std::unique_ptr<IPwmChannel> channel);

    bool start(unsigned int id);
    bool stop(unsigned int id) noexcept;
    bool startAll();
    void stopAll() noexcept;
    void allOff() noexcept;

    bool setDutyCycle(unsigned int id, double duty_percent);
    bool setFrequency(unsigned int id, std::uint32_t frequency);

    double dutyCycle(unsigned int id) const;
    std::uint32_t frequency(unsigned int id) const;

    IPwmChannel* channel(unsigned int id) noexcept;
    const IPwmChannel* channel(unsigned int id) const noexcept;

    std::shared_ptr<GpioChip> gpioChip() const noexcept { return gpio_chip_; }

private:
    std::shared_ptr<GpioChip> gpio_chip_;
    std::unordered_map<unsigned int, std::unique_ptr<IPwmChannel>> channels_;
};

} // namespace rpi::pwm
