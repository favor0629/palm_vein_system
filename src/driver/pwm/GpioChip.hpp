#pragma once

#include <cstdint>
#include <stdexcept>

struct LgpioHandleDeleter;


namespace rpi::pwm
{

class GpioChip
{
    public:
        explicit GpioChip(unsigned int chip = 0U);
        ~GpioChip() noexcept;

        GpioChip(const GpioChip&) = delete;
        GpioChip& operator=(const GpioChip&) = delete;

        int handle() const noexcept {return handle_;};

    private:
    int handle_ = -1;
};

}// namespace rpi::pwm