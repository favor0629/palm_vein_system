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

         //GPIO 芯片句柄属于独占资源，不能随意复制，防止多个对象错误关闭同一个句柄。
        GpioChip(const GpioChip&) = delete;    
        GpioChip& operator=(const GpioChip&) = delete;

        int handle() const noexcept {return handle_;};  //  获取底层 lgpio 句柄

    private:
    int handle_ = -1;
};

}// namespace rpi::pwm