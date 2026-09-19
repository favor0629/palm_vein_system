#include "GpioChip.hpp"

#include <lgpio.h>

#include <string>
#include <stdexcept>

namespace rpi::pwm
{

GpioChip::GpioChip(unsigned int chip)
    : handle_(lgGpiochipOpen(static_cast<int>(chip)))
{
    if (handle_ < 0) 
    {
        throw std::runtime_error("Unable to open gpiochip" + std::to_string(chip));
    }
}

GpioChip::~GpioChip() noexcept
{
    if (handle_ >= 0) 
    {
        (void)lgGpiochipClose(handle_);
        handle_ = -1;
    }
}

} // namespace rpi::pwm
