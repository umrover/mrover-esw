#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <type_traits>

#include <serial/uart.hpp>

namespace mrover {

#ifdef HAL_UART_MODULE_ENABLED

    /**
     * Send and receive plain structs over UART
     * 
     * Intended for use with the ST-LINK v3 Virtual COM Port (VCP)
     */
    class VCP {
    public:
        static constexpr size_t MAX_MESSAGE_SIZE = 254; // bytes

        VCP() = default;
        explicit VCP(UART* uart) : m_uart{uart} {}

        /**
         * Send message
         */
        template<typename T>
        auto send(T const& msg) -> void {
            static_assert(std::is_trivially_copyable_v<T> && sizeof(T) <= MAX_MESSAGE_SIZE);
            std::array<uint8_t, MAX_MESSAGE_SIZE + 2> frame{};
            size_t const n = cobs_encode({reinterpret_cast<uint8_t const*>(&msg), sizeof(T)}, frame);
            frame[n] = 0x00;
            m_uart->transmit(std::string_view{reinterpret_cast<char const*>(frame.data()), n + 1});
        }

        /**
         * Receive a message of type `T` into `msg`, skipping invalid messages
         *
         * @return true if `msg` was received
         */
        template<typename T>
        auto receive(T& msg) -> bool {
            static_assert(std::is_trivially_copyable_v<T> && sizeof(T) <= MAX_MESSAGE_SIZE);
            uint8_t byte;
            while (m_uart->receive_byte(byte)) {
                if (byte != 0x00) {
                    if (m_frame_len < m_frame.size()) {
                        m_frame[m_frame_len++] = byte;
                    } else {
                        m_frame_len = m_frame.size() + 1; // too long, skip to the next 0x00
                    }
                    continue;
                }
                size_t const len = m_frame_len;
                m_frame_len = 0;
                if (len > m_frame.size()) continue;

                std::array<uint8_t, MAX_MESSAGE_SIZE> decoded{};
                if (cobs_decode({m_frame.data(), len}, decoded) == sizeof(T)) {
                    std::memcpy(&msg, decoded.data(), sizeof(T));
                    return true;
                }
            }
            return false;
        }

    private:
        UART* m_uart{};
        std::array<uint8_t, MAX_MESSAGE_SIZE + 1> m_frame{};
        size_t m_frame_len{0};

        /**
         * Consistent Overhead Byte Stuffing (COBS)
         *
         * replace each 0x00 with the distance to the next one, so the frame contains no 0x00 bytes, which is the end delimeter
         *
         * @return encoded length (in.size() + 1)
         */
        static auto cobs_encode(std::span<uint8_t const> const in, std::span<uint8_t> const out) -> size_t {
            size_t code_idx = 0;
            size_t out_idx = 1;
            uint8_t code = 1;
            for (uint8_t const byte: in) {
                if (byte == 0x00) {
                    out[code_idx] = code;
                    code_idx = out_idx++;
                    code = 1;
                } else {
                    out[out_idx++] = byte;
                    ++code;
                }
            }
            out[code_idx] = code;
            return out_idx;
        }

        /**
         * COBS decode (decode COBS message)
         *
         * @return decoded length, or 0 if the frame is malformed or too long for `out`
         */
        static auto cobs_decode(std::span<uint8_t const> const in, std::span<uint8_t> const out) -> size_t {
            size_t in_idx = 0;
            size_t out_idx = 0;
            while (in_idx < in.size()) {
                uint8_t const code = in[in_idx++];
                if (in_idx + code - 1 > in.size() || out_idx + code - 1 > out.size()) return 0;
                for (uint8_t i = 1; i < code; ++i) out[out_idx++] = in[in_idx++];
                if (code != 0xFF && in_idx < in.size()) {
                    if (out_idx >= out.size()) return 0;
                    out[out_idx++] = 0x00;
                }
            }
            return out_idx;
        }
    };

#else  // HAL_UART_MODULE_ENABLED
    class __attribute__((unavailable("enable 'UART' in STM32CubeMX to use mrover::VCP"))) VCP {
    public:
        template<typename... Args>
        explicit VCP(Args&&... args) {}
    };
#endif // HAL_UART_MODULE_ENABLED

} // namespace mrover
