#pragma once

#include "PwmTypes.hpp"

namespace rpi::pwm
{

class IPwmChannel
{
    public:
        virtual ~IPwmChannel() = default;
        virtual bool start() = 0;
        virtual bool stop() = 0;

        virtual bool setDutyCycle(double duty_percent) = 0;
        virtual bool setFrequency(std::uint32_t frequency) = 0;

        virtual double dutyCycle() const noexcept = 0;
        virtual std::uint32_t frequency() const noexcept = 0;

        virtual bool isRunning() const noexcept = 0;
};
    
} // namespace rpi::pwm
