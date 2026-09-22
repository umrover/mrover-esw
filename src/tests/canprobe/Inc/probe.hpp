#pragma once

#include <cstdint>
#include <variant>

#include <MRoverCAN.hpp>
#include <logger.hpp>

#include "canprobe_config.hpp"


namespace mrover {

    class ProbeHandler {
        typedef void (*tx_exec_t)(uint32_t base_id, uint8_t const* data, std::size_t len);

        tx_exec_t m_message_tx_f{};
        canprobe_config_t* m_config_ptr{};
        uint32_t m_probe_counter{};

        template<typename can_msg_t>
            requires is_can_message<can_msg_t>
        auto send(can_msg_t const& message) const -> void {
            m_message_tx_f(can_msg_t::BASE_ID, message.msg_arr, sizeof(message.msg_arr));
        }

        template<typename T>
        auto handle(T const& _) const -> void {
            Logger::instance().debug("RX unhandled message type");
        }

        auto handle(ESWProbe const& msg) const -> void {
            Logger::instance().info("RX ESWProbe data=%u (0x%x)", msg.data, msg.data);
            send(ESWAck{msg.data});
            Logger::instance().info("TX ESWAck   data=%u", msg.data);
        }

        auto handle(ESWAck const& msg) const -> void {
            Logger::instance().info("RX ESWAck   data=%u (0x%x)", msg.data, msg.data);
        }

    public:
        ProbeHandler() = default;

        explicit ProbeHandler(tx_exec_t const& message_tx_f, canprobe_config_t* config)
            : m_message_tx_f{message_tx_f}, m_config_ptr{config} {}

        auto receive(MRoverCANMsg_t const& v) -> void {
            std::visit([this](auto&& value) -> auto {
                handle(value);
            },
                       v);
        }

        auto send_probe() -> void {
            send(ESWProbe{m_probe_counter});
            Logger::instance().info("TX ESWProbe data=%u", m_probe_counter);
            ++m_probe_counter;
        }
    };

} // namespace mrover
