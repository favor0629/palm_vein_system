#pragma once

#include "../driver/pwm/PwmController.hpp"

#include <cstdint>

namespace rpi::pwm 
{

class InfraredLampController 
{
public:
    explicit InfraredLampController(PwmController& pwm) : pwm_(pwm) {}

    bool start() { return pwm_.startAll(); }
    void stop() noexcept { pwm_.stopAll(); }

    // Application-level API. Brightness is intentionally expressed as percent.
    bool setBrightness(unsigned int channel, double brightness_percent);
    bool setPwmFrequency(unsigned int channel, std::uint32_t frequency);

    void allOff() noexcept;

private:
    PwmController& pwm_;
};

} // namespace rpi::pwm
