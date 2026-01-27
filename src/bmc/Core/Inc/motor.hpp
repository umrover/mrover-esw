#pragma once

#include <CANBus1.hpp>
#include <cinttypes>
#include <err.hpp>
#include <functional>
#include <hw/ad8418a.hpp>
#include <hw/hbridge.hpp>
#include <hw/limit_switch.hpp>
#include <pidf.hpp>
#include <units.hpp>
#include <variant>

#include "config.hpp"


namespace mrover {

    class Motor {
        using tx_exec_t = std::function<void(CANBus1Msg_t const& msg)>;

        HBridge m_hbridge;
        AD8418A m_current_sensor;
        LimitSwitch m_limit_a;
        LimitSwitch m_limit_b;

        tx_exec_t m_message_tx_f;

        bmc_config_t* m_config_ptr;
        mode_t m_mode;
        bmc_error_t m_error;
        float m_target;

        bool m_enabled{false};
        bool m_limit_a_hit{false};
        bool m_limit_b_hit{false};

        auto reset() -> void {
            m_mode = mode_t::STOPPED;
            m_error = bmc_error_t::NONE;
            m_target = 0.0f;
        }

        auto apply_limit(LimitSwitch& limit, bool& at_limit) -> void {
            if (limit.enabled()) {
                limit.update_limit_switch();
                if (m_target > 0.0f && limit.limit_forward()) {
                    m_target = 0.0f;
                    at_limit = true;
                } else if (m_target < 0.0f && limit.limit_backward()) {
                    m_target = 0.0f;
                    at_limit = true;
                } else {
                    at_limit = false;
                }
            }
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

                        Logger::instance().info("Setting target to %f", m_target);
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

            m_limit_a.init(
                m_config_ptr->get<bmc_config_t::lim_a_en>(),
                m_config_ptr->get<bmc_config_t::lim_a_active_high>(),
                m_config_ptr->get<bmc_config_t::lim_a_use_readjust>(),
                m_config_ptr->get<bmc_config_t::lim_a_is_forward>(),
                m_config_ptr->get<bmc_config_t::limit_a_position>()
            );

            m_limit_b.init(
                m_config_ptr->get<bmc_config_t::lim_b_en>(),
                m_config_ptr->get<bmc_config_t::lim_b_active_high>(),
                m_config_ptr->get<bmc_config_t::lim_b_use_readjust>(),
                m_config_ptr->get<bmc_config_t::lim_b_is_forward>(),
                m_config_ptr->get<bmc_config_t::limit_b_position>()
            );

            // TODO(eric) provide a mechanism to reset the FDCAN filter
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
            if (!msg.target_valid) return;
            switch (m_mode) {
                case mode_t::STOPPED:
                case mode_t::FAULT:
                    m_target = 0.0f;
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
                LimitSwitch const& limit_a,
                LimitSwitch const& limit_b,
                tx_exec_t const& message_tx_f,
                bmc_config_t* config) : m_hbridge{motor_driver},
                                        m_current_sensor{current_sensor},
                                        m_limit_a{limit_a},
                                        m_limit_b{limit_b},
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
            // m_current_sensor.update_sensor();

            m_message_tx_f(BMCMotorState{
                    static_cast<uint8_t>(m_mode),  // mode
                    static_cast<uint8_t>(m_error), // fault-code
                    0.0,                           // position
                    0.0,                           // velocity
                    0,                             // timestamp
                    m_limit_a_hit,                 // limit_a_set
                    m_limit_b_hit,                 // limit_b_set
                    0,                             // is_stalled
                    0                              //m_current_sensor.current()     // current
            });

            // Logger::instance().info("Current: %f", m_current_sensor.current());
        }

        auto drive_output() -> void {
            // update limit switch state (modifies m_target if stop needed)
            apply_limit(m_limit_a, m_limit_a_hit);
            apply_limit(m_limit_b, m_limit_b_hit);
            // TODO(eric) read quad encoders
            // TODO(eric) read abs encoders
            write_output_pwm();
        }

        auto tx_watchdog_lapsed() -> void {
            m_mode = mode_t::FAULT;
            m_error = bmc_error_t::WWDG_EXPIRED;
        }

        auto reset_wwdg() -> void {
            if (m_mode == mode_t::FAULT && m_error == bmc_error_t::WWDG_EXPIRED) {
                m_mode = mode_t::STOPPED;
                m_error = bmc_error_t::NONE;
            }
        }
    };
} // namespace mrover
