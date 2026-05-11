#pragma once

#include <MRoverCAN.hpp>
#include <cinttypes>
#include <hw/limit_switch.hpp>
#include <pidf.hpp>
#include <sys.hpp>
#include <variant>

#include "lim_config.hpp"


namespace mrover {

    class LimitHandler {
        typedef void (*tx_exec_t)(MRoverCANMsg_t const& msg);

        std::optional<LimitSwitch> m_limit_a;
        std::optional<LimitSwitch> m_limit_b;

        tx_exec_t m_message_tx_f{};
        lim_config_t* m_config_ptr{};
        bool m_limit_a_hit{false};
        bool m_limit_b_hit{false};

        /**
         * Initializes the motor as the configuration defines.
         *
         * Should be called after configuration is updated.
         */
        auto init() -> void {
            System::InterruptGuard guard{};

            // init limit switches
            bool present = m_config_ptr->get<lim_config_t::lim_a_present>();
            bool en = m_config_ptr->get<lim_config_t::lim_a_en>();
            bool active_high = m_config_ptr->get<lim_config_t::lim_a_active_high>();
            m_limit_a->init(present, en, active_high);

            present = m_config_ptr->get<lim_config_t::lim_b_present>();
            en = m_config_ptr->get<lim_config_t::lim_b_en>();
            active_high = m_config_ptr->get<lim_config_t::lim_b_active_high>();
            m_limit_b->init(present, en, active_high);
        }

        template<typename T>
        auto handle(T const& _) const -> void {
        }

        auto handle(ESWProbe const& msg) const -> void {
            // acknowledge probe
            m_message_tx_f(ESWAck{msg.data});
        }

        auto handle(ESWConfigCmd const& msg) -> void {
            // input can either be a request to set a value (apply is set) or read a value (apply not set)
            if (msg.apply) {
                if (m_config_ptr->set_raw(msg.address, msg.value)) {
                    // re-initialize after configuration is modified
                    init();
                }
            } else {
                // send data back as an acknowledgement of the request
                if (uint32_t val{}; m_config_ptr->get_raw(msg.address, val)) {
                    m_message_tx_f(ESWAck{val});
                }
            }
        }

        auto handle(LIMResetCmd const& msg) -> void {
            System::reset();
        }

    public:
        LimitHandler() = default;

        explicit LimitHandler(
                LimitSwitch const& limit_a,
                LimitSwitch const& limit_b,
                tx_exec_t const& message_tx_f,
                lim_config_t* config) : m_message_tx_f{message_tx_f},
                                        m_config_ptr{config} {
            m_limit_a.emplace(limit_a);
            m_limit_b.emplace(limit_b);
            init();
        }

        auto receive(MRoverCANMsg_t const& v) -> void {
            std::visit([this](auto&& value) -> auto {
                handle(value);
            },
                       v);
        }

        auto send_state() -> void {
            m_limit_a->update_limit_switch();
            m_limit_b->update_limit_switch();
            m_limit_a_hit = m_limit_a->pressed();
            m_limit_b_hit = m_limit_b->pressed();
            m_message_tx_f(LIMState{
                    m_limit_a_hit, // limit_a_set
                    m_limit_b_hit  // limit_b_set
            });
        }
    };
} // namespace mrover
