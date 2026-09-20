#include "PwmController.hpp"

#include <stdexcept>

#include "../../../test/debug.hpp"

namespace rpi::pwm {

PwmController::PwmController(unsigned int gpio_chip)
    : gpio_chip_(std::make_shared<GpioChip>(gpio_chip))
{
}

bool PwmController::addChannel(unsigned int id, std::unique_ptr<IPwmChannel> channel)
{
    /**
     * If the channel pointer is null or the channel ID already exists in the map, return false.
     * This prevents adding a null channel or overwriting an existing channel with the same ID.
     */
    if (!channel || channels_.find(id) != channels_.end()) {
        DEBUG_ERROR("PWM", "Failed to add channel " << id
            << ": channel is null or ID already exists");
        return false;
    }

    channels_.emplace(id, std::move(channel));
    return true;
}

IPwmChannel* PwmController::channel(unsigned int id) noexcept
{
    const auto it = channels_.find(id);
    return (it == channels_.end()) ? nullptr : it->second.get();
}

const IPwmChannel* PwmController::channel(unsigned int id) const noexcept
{
    const auto it = channels_.find(id);
    return (it == channels_.end()) ? nullptr : it->second.get();
}

bool PwmController::start(unsigned int id)
{
    IPwmChannel* ch = channel(id);
    return ch != nullptr && ch->start();
}

bool PwmController::stop(unsigned int id) noexcept
{
    IPwmChannel* ch = channel(id);
    if (ch == nullptr) 
    {
        return false;
    }
    ch->stop();
    return true;
}

bool PwmController::startAll()
{
    for (auto& [id, ch] : channels_) 
    {
        if (!ch->start()) 
        {
            DEBUG_ERROR("PWM", "Failed to start PWM channel " << id);
            // Fail-safe behavior: anything we already started is turned off.
            for (auto& [startedId, startedCh] : channels_) 
            {
                if (startedId != id) 
                {
                    startedCh->stop();
                }
            }
            return false;
        }
    }

    return true;
}

void PwmController::stopAll() noexcept
{
    for (auto& [id, ch] : channels_) 
    {
        (void)id;
        ch->stop();
    }
}

void PwmController::allOff() noexcept
{
    for (auto& [id, ch] : channels_) 
    {
        (void)id;
        (void)ch->setDutyCycle(0.0);
    }
}

bool PwmController::setDutyCycle(unsigned int id, double duty_percent)
{
    IPwmChannel* ch = channel(id);
    if (ch == nullptr)
    {
        DEBUG_ERROR("PWM", "Unknown PWM channel " << id << " while setting duty cycle");
        return false;
    }
    if (!ch->setDutyCycle(duty_percent))
    {
        DEBUG_ERROR("PWM", "Failed to set channel " << id
            << " duty cycle to " << duty_percent << "%");
        return false;
    }
    return true;
}

bool PwmController::setFrequency(unsigned int id, std::uint32_t frequency)
{
    IPwmChannel* ch = channel(id);
    return ch != nullptr && ch->setFrequency(frequency);
}

double PwmController::dutyCycle(unsigned int id) const
{
    const IPwmChannel* ch = channel(id);
    if (ch == nullptr) 
    {
        throw std::out_of_range("Unknown PWM channel id");
    }
    return ch->dutyCycle();
}

std::uint32_t PwmController::frequency(unsigned int id) const
{
    const IPwmChannel* ch = channel(id);
    if (ch == nullptr) 
    {
        throw std::out_of_range("Unknown PWM channel id");
    }
    return ch->frequency();
}

} // namespace rpi::pwm
