#pragma once

#include "GpioChip.hpp"
#include "IPwmChannel.hpp"

#include <cstdint>
#include <memory>
#include <utility>


/**
 *               stopped
                    │
                  start
                    ▼
             ┌──────────────┐
             │    running   │
             └──────┬───────┘
                    │
        ┌───────────┴───────────┐
        ▼                       ▼
  static output             PWM output
 pwm_active=false          pwm_active=true
        │                       │
      0/100%                 0~100%
        │                       │
        └───────────┬───────────┘
                    │
                   stop
                    ▼
                 stopped
 */
namespace rpi::pwm {

class SoftwarePwmChannel final : public IPwmChannel 
{
public:
    SoftwarePwmChannel(std::shared_ptr<GpioChip> gpio_chip,
                       unsigned int gpio,
                       std::uint32_t frequency = 1000,
                       double duty_percent = 0.0);

    ~SoftwarePwmChannel() override;

    /**
     * SoftwarePwmChannel 是独占资源，不能随意复制，防止多个对象错误关闭同一个 GPIO。
     * 拷贝构造被删除
     */
    SoftwarePwmChannel(const SoftwarePwmChannel&) = delete;
    SoftwarePwmChannel& operator=(const SoftwarePwmChannel&) = delete;

    bool start() override;
    bool stop() noexcept override;

    bool setDutyCycle(double duty_percent) override;
    bool setFrequency(std::uint32_t frequency) override;

    double dutyCycle() const noexcept override { return duty_percent_; }
    std::uint32_t frequency() const noexcept override { return frequency_; }
    bool isRunning() const noexcept override { return running_; }

    unsigned int gpio() const noexcept { return gpio_; }

private:
    bool applyPwm();
    bool applyStaticLevel(int level);

    std::shared_ptr<GpioChip> gpio_chip_;
    unsigned int gpio_;
    std::uint32_t frequency_;           // pwm 频率
    double duty_percent_;               // pwm占空比
    bool running_ = false;              //PWM 是否正在运行
    bool claimed_ = false;              //GPIO 是否已经被当前对象成功 claim（申请/占用）
    bool pwm_active_ = false;           //当前 GPIO 是否正在由 lgTxPwm() 提供 PWM 波形
};

} // namespace rpi::pwm