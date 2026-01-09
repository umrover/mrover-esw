#pragma once

#include <variant>
#include <hw/hbridge.hpp>
#include <err.hpp>
#include <CANBus1.hpp>
#include <cinttypes>
#include <functional>
#include <pidf.hpp>
#include <units.hpp>

#include "config.hpp"


namespace mrover {

    class Motor {
        using tx_exec_t = std::function<void(CANBus1Msg_t const& msg)>;

        HBridge m_hbridge;
        tx_exec_t m_message_tx_f;

        bmc_config_t* m_config_ptr;
        mode_t m_mode;
        bmc_error_t m_error;
        Percent m_target;

        bool m_enabled = false;

        auto reset() -> void {
            m_mode = mode_t::STOPPED;
            m_error = bmc_error_t::NONE;
            m_target = 0.0f;
        }

        auto write_output_pwm() -> void {
            if (m_enabled) {
                m_hbridge.write(m_target);
            }
        }

        /**
         * Initializes the motor as the configuration defines.
         *
         * Should be called after configuration is updated.
         */
        auto init() -> void {
            Logger::get_instance()->info("BMC Initialized with CAN ID 0x%02" PRIX32, m_config_ptr->get_can_id());
            m_enabled = m_config_ptr->get_motor_en();
            m_hbridge.change_inverted(m_config_ptr->get_motor_inv());
            m_hbridge.change_max_pwm(m_config_ptr->get_max_pwm());
            // TODO(eric) add limit switches
            // TODO(eric) add quad encoders
            // TODO(eric) add absolute encoders
        }

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
            Logger::get_instance()->debug("Mode set to %u", m_mode);
        }

        auto handle(BMCTargetCmd const& msg) -> void {
            if (!msg.target_valid) return;
            m_target = msg.target;
            Logger::get_instance()->debug("Target set to %f", m_target);
        }

        auto handle(BMCConfigCmd const& msg) -> void {
            if (msg.apply) {
                if (m_config_ptr->set(msg.address, msg.value)) {
                    Logger::get_instance()->debug("Written 0x%08" PRIX32 "to address 0x%02" PRIX32, msg.value, msg.address);
                    // re-initialize after config is modified
                    init();
                } else {
                    Logger::get_instance()->warn("Register 0x%02" PRIX32 " does not exist, write failed", msg.address);
                }
            } else {
                if (uint32_t val{}; m_config_ptr->get(msg.address, val)) {
                    m_message_tx_f(BMCAck{val});
                } else {
                    Logger::get_instance()->warn("Register 0x%02" PRIX32 " does not exist, read failed", msg.address);
                }
            }
        }

        auto handle(BMCResetCmd const& msg) -> void {
            reset();
            Logger::get_instance()->info("BMC Reset Received");
        }

    public:
        Motor() = default;

        explicit Motor(
            HBridge const& motor_driver,
            tx_exec_t const& message_tx_f,
            bmc_config_t* config
        ) :
            m_hbridge{motor_driver},
            m_message_tx_f{message_tx_f},
            m_config_ptr{config},
            m_mode{mode_t::STOPPED},
            m_error{bmc_error_t::NONE},
            m_target{0.0f}
        {
            reset();
            init();
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
