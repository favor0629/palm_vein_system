#include "InfraredLampController.hpp"

#include <stdexcept>

namespace rpi::pwm {

bool InfraredLampController::setBrightness(unsigned int channel, double brightness_percent)
{
    validateDuty(brightness_percent);
    return pwm_.setDutyCycle(channel, brightness_percent);
}

bool InfraredLampController::setPwmFrequency(unsigned int channel, std::uint32_t frequency)
{
    validateFrequency(frequency);
    return pwm_.setFrequency(channel, frequency);
}

void InfraredLampController::allOff() noexcept
{
    // Setting duty to zero keeps all configured channels available for later use.
    pwm_.allOff();
}

} // namespace rpi::pwm
