#pragma once

#include <CANBus1.hpp>
#include <cinttypes>
#include <err.hpp>
#include <functional>
#include <hw/ad8418a.hpp>
#include <hw/hbridge.hpp>
#include <pidf.hpp>
#include <units.hpp>
#include <variant>

#include "config.hpp"


namespace mrover {

    class Motor {
        using tx_exec_t = std::function<void(CANBus1Msg_t const& msg)>;

        HBridge m_hbridge;
        AD8418A m_current_sensor;
        tx_exec_t m_message_tx_f;

        bmc_config_t* m_config_ptr;
        mode_t m_mode;
        bmc_error_t m_error;
        float m_target;

        bool m_enabled = false;

        auto reset() -> void {
            m_mode = mode_t::STOPPED;
            m_error = bmc_error_t::NONE;
            m_target = 0.0f;
        }

        auto write_output_pwm() -> void {
            if (m_enabled) {
                switch (m_mode) {
                    case mode_t::STOPPED:
                        if (m_hbridge.is_on()) m_hbridge.stop();
                        break;
                    case mode_t::FAULT:
                        if (m_hbridge.is_on()) m_hbridge.stop();
                        break;
                    case mode_t::THROTTLE:
                        if (!m_hbridge.is_on()) m_hbridge.start();
                        m_hbridge.write(m_target);
                        break;
                    case mode_t::POSITION:
                        if (m_hbridge.is_on()) m_hbridge.stop();
                        // TODO(eric) PID calcs here
                        break;
                    case mode_t::VELOCITY:
                        if (m_hbridge.is_on()) m_hbridge.stop();
                        // TODO(eric) PID calcs here
                        break;
                }
            }
        }

        /**
         * Initializes the motor as the configuration defines.
         *
         * Should be called after configuration is updated.
         */
        auto init() -> void {
            Logger::instance().info("BMC Initialized with CAN ID 0x%02" PRIX32, m_config_ptr->get<bmc_config_t::can_id>());
            m_enabled = m_config_ptr->get<bmc_config_t::motor_en>();
            m_hbridge.set_inverted(m_config_ptr->get<bmc_config_t::motor_inv>());
            m_hbridge.set_max_pwm(m_config_ptr->get<bmc_config_t::max_pwm>());
            m_current_sensor.init(get_current_sensor_options());
            // TODO(eric) add limit switches
            // TODO(eric) add quad encoders
            // TODO(eric) add absolute encoders
        }

        template<typename T>
        auto handle(T const& _) const -> void {
            Logger::instance().debug("Received Unhandled Message Type");
        }

        auto handle(BMCProbe const& msg) const -> void {
            // acknowledge probe
            Logger::instance().info("BMC Probed with data %u", msg.data);
            m_message_tx_f(BMCAck{msg.data});
        }

        auto handle(BMCModeCmd const& msg) -> void {
            // stop if not enabled, consume mode only if enabled
            if (!msg.enable)
                m_mode = mode_t::STOPPED;
            else {
                m_mode = static_cast<mode_t>(msg.mode);
            }
            Logger::instance().info("Mode set to %u", m_mode);
        }

        auto handle(BMCTargetCmd const& msg) -> void {
            Logger::instance().info("Received Target Command");
            if (!msg.target_valid) return;
            Logger::instance().info("Received Valid Target Command");
            switch (m_mode) {
                case mode_t::STOPPED:
                case mode_t::FAULT:
                    m_target = 0.0f;
                    Logger::instance().info("STOPPED | FAULT: Set Target to %.2f", m_target);
                    break;
                case mode_t::THROTTLE:
                case mode_t::POSITION:
                case mode_t::VELOCITY:
                    m_target = msg.target;
                    Logger::instance().info("Set Target to %.2f", m_target);
                    break;
            }
        }

        auto handle(BMCConfigCmd const& msg) -> void {
            // input can either be a request to set a value (apply is set) or read a value (apply not set)
            if (msg.apply) {
                if (m_config_ptr->set_raw(msg.address, msg.value)) {
                    Logger::instance().info("Written 0x%08" PRIX32 " to address 0x%02" PRIX32, msg.value, msg.address);
                    // re-initialize after configuration is modified
                    init();
                } else {
                    Logger::instance().warn("Register 0x%02" PRIX32 " does not exist, write failed", msg.address);
                }
            } else {
                // send data back as an acknowledgement of the request
                if (uint32_t val{}; m_config_ptr->get_raw(msg.address, val)) {
                    m_message_tx_f(BMCAck{val});
                } else {
                    Logger::instance().warn("Register 0x%02" PRIX32 " does not exist, read failed", msg.address);
                }
            }
        }

        auto handle(BMCResetCmd const& msg) -> void {
            reset();
            Logger::instance().info("BMC Reset Received");
        }

    public:
        Motor() = default;

        explicit Motor(
                HBridge const& motor_driver,
                AD8418A const& current_sensor,
                tx_exec_t const& message_tx_f,
                bmc_config_t* config) : m_hbridge{motor_driver},
                                        m_current_sensor{current_sensor},
                                        m_message_tx_f{message_tx_f},
                                        m_config_ptr{config},
                                        m_mode{mode_t::STOPPED},
                                        m_error{bmc_error_t::NONE},
                                        m_target{0.0f} {
            reset();
            init();
        }

        auto receive(CANBus1Msg_t const& v) -> void {
            std::visit([this](auto&& value) {
                handle(value);
            },
                       v);
        }

        auto send_state() -> void {
            m_current_sensor.update_sensor();

            m_message_tx_f(BMCMotorState{
                    static_cast<uint8_t>(m_mode),  // mode
                    static_cast<uint8_t>(m_error), // fault-code
                    0.0,                           // position
                    0.0,                           // velocity
                    0,                             // timestamp
                    0,                             // limit_a_set
                    0,                             // limit_b_set
                    0,                             // is_stalled
                    m_current_sensor.current()     // current
            });
        }

        auto drive_output() -> void {
            // TODO(eric) read limit switch state
            // TODO(eric) read quad encoders
            // TODO(eric) read abs encoders
            write_output_pwm();
        }

        auto tx_watchdog_lapsed() -> void {
            m_mode = mode_t::FAULT;
            m_error = bmc_error_t::WWDG_EXPIRED;
        }
    };
} // namespace mrover
