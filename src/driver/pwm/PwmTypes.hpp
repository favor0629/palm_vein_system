#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace rpi::pwm
{

enum class PwmBackend
{
    Hardware,
    Software
};

struct PwmConfig
{
    unsigned int gpio = 0;      // BCM GPIO number
    std::uint32_t frequencyHz = 1000;
    double duty_percent = 0.0;       // [0, 100]

    PwmBackend backend = PwmBackend::Software;  // 默认为软件pwm
    unsigned int hw_channel = 0;// Linux PWM channel inside pwmchip0.
};

inline void validateDuty(double duty_percent)
{
    if(duty_percent < 0.0 || duty_percent > 100.0)
    {
        throw std::invalid_argument("PWM duty cycle must be in [0, 100] percent");
    }
}

inline void validateFrequency(std::uint32_t frequency)
{
    if(frequency == 0U)
    {
        throw std::invalid_argument("PWM frequency must be greater than 0 Hz");
    }
}

inline std::string backendToString(PwmBackend backend)
{
    switch (backend)
    {
        case PwmBackend::Hardware:
        {
            return "hardware";
        }
        case PwmBackend::Software:
        {
            return "software";
        }
    }
    return "unknown";
}
}       // namespace rpi::pwm