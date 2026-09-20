#include "GpioChip.hpp"

#include <lgpio.h>

#include <string>
#include <stdexcept>

#include "../../../test/debug.hpp"

namespace rpi::pwm
{

GpioChip::GpioChip(unsigned int chip)
    : handle_(lgGpiochipOpen(static_cast<int>(chip)))
{
    if (handle_ < 0) 
    {
        DEBUG_ERROR("GPIO", "Unable to open gpiochip" << chip);
        throw std::runtime_error("Unable to open gpiochip" + std::to_string(chip));
    }
}

GpioChip::~GpioChip() noexcept
{
    if (handle_ >= 0) 
    {
        const int result = lgGpiochipClose(handle_);
        if (result < 0)
        {
            DEBUG_WARN("GPIO", "Failed to close gpiochip, ret=" << result);
        }
        handle_ = -1;
    }
}

} // namespace rpi::pwm
