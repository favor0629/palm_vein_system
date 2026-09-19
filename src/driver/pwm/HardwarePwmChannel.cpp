#include "HardwarePwmChannel.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>


namespace rpi::pwm
{

namespace 
{
constexpr std::uint64_t NANOSECONDS_PER_SECOND = 1'000'000'000ULL;

/**
 * Calculates the period in nanoseconds from the given frequency in Hz.
 *
 * @param frequency The frequency in Hz.
 * @return The period in nanoseconds.
 */
std::uint64_t periodNsFromFrequency(std::uint32_t frequency)
{
    if(frequency == 0U)
    {
        throw std::invalid_argument("PWM frequency must be greater than 0 Hz");
    }

    return NANOSECONDS_PER_SECOND / static_cast<std::uint64_t>(frequency);
}

/**
 * Calculates the duty cycle in nanoseconds from the given period in nanoseconds and duty cycle percentage.
 *
 * @param period_ns The period in nanoseconds.
 * @param duty_percent The duty cycle percentage.
 * @return The duty cycle in nanoseconds.
 */
std::uint64_t dutyNsFromPercent(std::uint64_t period_ns, double duty_percent)
{
    const double duty = std::clamp(duty_percent, 0.0, 100.0);
    const double duty_ns = static_cast<double>(period_ns) * duty / 100.0;
    return static_cast<std::uint64_t>(duty_ns);
}

/**
 * Checks if the specified path exists in the filesystem.
 *
 * @param path The path to check.
 * @return True if the path exists, false otherwise.
 */
bool pathExists(const std::filesystem::path& path)
{
    std::error_code ec;
    return std::filesystem::exists(path, ec) && !ec;
}

}  // namespace


HardwarePwmChannel::HardwarePwmChannel(unsigned int gpio,
                                       unsigned int hw_channel,
                                       std::uint32_t frequency,
                                       double duty_percent,
                                       std::filesystem::path pwm_chip)
    :gpio_(gpio),
     hw_channel_(hw_channel),
     frequency_(frequency),
     duty_percent_(duty_percent),
     pwm_chip_(std::move(pwm_chip))
{
    if(hw_channel_ > 1)
    {
        throw std::invalid_argument("Hardware PWM channel must be 0 or 1");
    }

    validateDuty(duty_percent_);
    validateFrequency(frequency_);
}

HardwarePwmChannel::~HardwarePwmChannel()
{
    stop();
    unexportIfOwned();
}

/**
 * Returns the filesystem path for the PWM channel.
 *
 * @return The filesystem path for the PWM channel.
 */
std::filesystem::path HardwarePwmChannel::channelPath() const
{
    return pwm_chip_ / ("pwm" + std::to_string(hw_channel_));

}

/**
 * Ensures that the PWM channel is exported in the filesystem.
 *
 * @return True if the channel is exported, false otherwise.
 */
bool HardwarePwmChannel::ensureExported()
{
    if(!pathExists(pwm_chip_))
    {
        return false;
    }

    if(pathExists(channelPath()))
    {
        return true;
    }

    std::ofstream export_file(pwm_chip_ / "export");
    if(!export_file)
    {
        return false;
    }
    export_file << hw_channel_;
    export_file.close();

    for(int attempt = 0; attempt < 100; ++attempt)
    {
        if(pathExists(channelPath()))
        {
            exported_by_us_ = true;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
}


/**
 * Writes a string value to a specified attribute of the PWM channel.
 *
 * @param name The name of the attribute to write to.
 * @param value The value to write to the attribute.
 * @return True if the write was successful, false otherwise.
 */
bool HardwarePwmChannel::writeAttribute(const std::string& name,
                                        const std::string& value) const noexcept
{
    try
    {
        std::ofstream file(channelPath() / name);
        if(!file)
        {
            return false;
        }
        file << value;
        return static_cast<bool>(file);
    } 
    catch (...)
    {
        return false;
    }
}

/**
 * Writes an unsigned 64-bit integer value to a specified attribute of the PWM channel.
 *
 * @param name The name of the attribute to write to.
 * @param value The value to write to the attribute.
 * @return True if the write was successful, false otherwise.
 */
bool HardwarePwmChannel::writeUInt64(const std::string &name, std::uint64_t value) const noexcept
{
    return writeAttribute(name, std::to_string(value));
}


bool HardwarePwmChannel::applyConfiguration()
{
    if (!ensureExported())
    {
        return false;
    }

    const std::uint64_t period_ns = periodNsFromFrequency(frequency_);
    const std::uint64_t duty_ns = dutyNsFromPercent(period_ns, duty_percent_);

    // Safe reconfiguration order: disable -> duty=0 -> period -> requested duty -> enable.
    if (!writeAttribute("enable", "0")) 
    {
        return false;
    }

    if (!writeUInt64("duty_cycle", 0U)) 
    {
        return false;
    }

    if (!writeUInt64("period", period_ns)) 
    {
        return false;
    }

    if (!writeUInt64("duty_cycle", std::min(duty_ns, period_ns))) 
    {
        return false;
    }

    if (!writeAttribute("enable", "1")) 
    {
        return false;
    }

    return true;
}


bool HardwarePwmChannel::start()
{
    if(running_)
    {
        return true;
    }

    if(!applyConfiguration())
    {
        return false;
    }

    // 设置正在运行
    running_ = true;

    return true;
}


bool HardwarePwmChannel::stop() noexcept
{
    // If the channel is not running, there's nothing to stop.
    if(!running_)
    {
        return true;
    }

    const bool disabled = writeAttribute("enable", "0");
    const bool duty_cleared = writeUInt64("duty_cycle", 0U);

    running_ = false;
    return disabled && duty_cleared;
}



bool HardwarePwmChannel::setDutyCycle(double duty_percent)
{
    try 
    {
        validateDuty(duty_percent);
    } 
    catch (const std::invalid_argument&) 
    {
        return false;
    }

    if (!running_) 
    {
        duty_percent_ = duty_percent;
        return true;
    }

    const std::uint64_t period_ns = periodNsFromFrequency(frequency_);
    const std::uint64_t duty_ns = dutyNsFromPercent(period_ns, duty_percent);

    // Disable before reprogramming to avoid transient invalid duty > period states.
    if (!writeAttribute("enable", "0")) 
    {
        return false;
    }
    if (!writeUInt64("duty_cycle", std::min(duty_ns, period_ns))) 
    {
        (void)writeAttribute("enable", "1");
        return false;
    }
    if (!writeAttribute("enable", "1")) 
    {
        return false;
    }

    duty_percent_ = duty_percent;
    return true;
}



bool HardwarePwmChannel::setFrequency(std::uint32_t frequency)
{
    try 
    {
        validateFrequency(frequency);
    } 
    catch (const std::invalid_argument&) 
    {
        return false;
    }

    if (!running_) 
    {
        frequency_ = frequency;
        return true;
    }

    const std::uint32_t old_frequency = frequency_;
    frequency_ = frequency;

    /**
     * 如果新的频率配置失败，则恢复旧的频率配置，并尝试重新应用旧的配置。
     */
    if (!applyConfiguration()) 
    {
        frequency_ = old_frequency ;
        (void)applyConfiguration();
        return false;
    }

    return true;
}


void HardwarePwmChannel::unexportIfOwned() noexcept
{
    if (!exported_by_us_) 
    {
        return;
    }

    try 
    {
        std::ofstream unexport_file(pwm_chip_ / "unexport");
        if (unexport_file) 
        {
            unexport_file << hw_channel_;
        }
    } 
    catch (...) 
    {
        // Destructors must not throw.
    }
    exported_by_us_ = false;
}








}