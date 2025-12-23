#pragma once

#include <variant>
#include <hw/hbridge.hpp>
#include <err.hpp>
#include <CANBus1.hpp>
#include <functional>
#include <pidf.hpp>
#include <units.hpp>

#include "config.hpp"


namespace mrover {

    class Motor {
        using send_hook_t = std::function<void(CANBus1Msg_t const& msg)>;

        HBridge m_hbridge;
        send_hook_t m_message_tx_f;

        mode_t m_mode;
        bmc_error_t m_error;
        float m_target;

        template <typename T>
        auto handle(T const& _) const -> void {
            Logger::get_instance()->debug("Received Unhandled Message Type");
        }

        auto handle(BMCProbe const& msg) const -> void {
            // acknowledge probe
            Logger::get_instance()->info("BMC Probed with data %u", msg.data);
            m_message_tx_f(BMCAck{msg.data});
        }

        auto handle(BMCModeCmd const& msg) -> void {
            // stop if not enabled, consume mode only if enabled
            if (!msg.enable) m_mode = mode_t::STOPPED;
            else m_mode = static_cast<mode_t>(msg.mode);
        }

        auto handle(BMCTargetCmd const& msg) -> void {
            if (!msg.target_valid) return;
            m_target = msg.target;
        }

        auto handle(BMCConfigCmd const& msg) -> void {
        }

        auto handle(BMCResetCmd const& msg) -> void {
        }

    public:
        Motor() = default;

        explicit Motor(
            HBridge const& motor_driver,
            send_hook_t const& message_tx_f
        ) :
            m_hbridge{motor_driver},
            m_message_tx_f{message_tx_f},
            m_mode{mode_t::STOPPED},
            m_error{bmc_error_t::NONE},
            m_target{0.0f}
        {
            m_hbridge.change_max_pwm(100);
            m_hbridge.write(0);
        }

        auto receive(CANBus1Msg_t const& v) -> void {
            std::visit([this](auto&& value) {
                handle(value);
            }, v);
        }

        auto send_state() const -> void {
            m_message_tx_f(BMCMotorState{
                static_cast<uint8_t>(m_mode), // mode
                static_cast<uint8_t>(m_error), // fault-code
                0.0, // position
                0.0, // velocity
                0, // timestamp
                0, // limit_a_set
                0, // limit_b_set
                0, // is_stalled
                0.0 // current
            });
        }
    };
} // namespace mrover
