#pragma once

#include <cstdint>
#include <string_view>
#include <span>
#include <atomic>

#include <util.hpp>

#include "main.h"

namespace mrover {

#ifdef HAL_UART_MODULE_ENABLED

    /**
     * UART abstraction class.
     *
     * Implementations of this class can be synchronous or asynchronous.
     * Asynchronous implementations must declare a Memory -> Peripheral DMA channel (tx) that is a byte wide.
     */
    class UART {
    public:
        static constexpr size_t TX_BUF_SIZE = 1024;

        struct Options {
            Options() {}
            uint32_t timeout_ms{100};
            bool use_dma{false};
        };

        UART() = default;

        explicit UART(
            UART_HandleTypeDef* huart,
            Options const& options = Options()
        ) :
            m_huart{huart},
            m_options{options}
        {}

        UART(const UART&) = delete;
        UART& operator=(const UART&) = delete;

        UART(UART&& other) noexcept { *this = std::move(other); }
        UART& operator=(UART&& other) noexcept {
            if (this != &other) {
                m_huart = other.m_huart;
                m_options = other.m_options;
                m_head = other.m_head.load();
                m_tail = other.m_tail.load();
                m_is_busy = other.m_is_busy.load();
                register_dma_instance();
            }
            return *this;
        }

        /**
         * Transmit provided serial data.
         * Can be synchronous or asynchronous depending on configured options of instance.
         * @param data Data to be sent on wire
         */
        auto transmit(std::string_view const data) const -> void {
            if (!m_options.use_dma) {
                auto* ptr = reinterpret_cast<uint8_t const*>(data.data());
                HAL_UART_Transmit(m_huart, const_cast<uint8_t*>(ptr), static_cast<uint16_t>(data.size()), m_options.timeout_ms);
                return;
            }

            for (char const c : data) {
                size_t const next = (m_head + 1) % TX_BUF_SIZE;
                if (next != m_tail) {
                    m_ring_buffer[m_head] = static_cast<uint8_t>(c);
                    m_head = next;
                }
            }
            resume_dma_transmission();
        }

        /**
         * Transmit single byte of serial data.
         * @param byte Byte to be sent on wire
         */
        auto transmit(uint8_t const byte) const -> void {
            transmit(std::string_view(reinterpret_cast<const char*>(&byte), 1));
        }

        /**
         * Callback to enable asynchronous data transmission via DMA.
         *
         * This function MUST be called from `HAL_UART_TxCpltCallback` for correct operation.
         * Also ensure global interrupts are enabled for the UART peripheral.
         */
        auto handle_tx_complete() const -> void {
            m_is_busy = false;
            resume_dma_transmission();
        }

        /**
         * Receive bytes and emplace into buffer (synchronous).
         * @param buffer Destination buffer for received bytes
         * @return HAL Status
         */
        [[nodiscard]] auto receive(std::span<uint8_t> buffer) const -> bool {
            if (buffer.empty()) return true;

            auto const status = HAL_UART_Receive(m_huart, buffer.data(), static_cast<uint16_t>(buffer.size()), m_options.timeout_ms);
            return status == HAL_OK;
        }

        /**
         * Receive byte and emplace into buffer (synchronous).
         * @param out_byte Destination buffer byte for received bytes
         * @return HAL Status
         */
        [[nodiscard]] auto receive_byte(uint8_t& out_byte) const -> bool {
            auto const status = HAL_UART_Receive(m_huart, &out_byte, 1, m_options.timeout_ms);
            return status == HAL_OK;
        }

        /**
         * Reset the UART peripheral (abort current transaction).
         */
        auto reset() const -> void {
            HAL_UART_Abort(m_huart);
        }

        /**
         * Get the HAL UART handle under the instance.
         * @return The underlying UART handle
         */
        [[nodiscard]] auto handle() const -> UART_HandleTypeDef* {
            return m_huart;
        }

    private:
        UART_HandleTypeDef* m_huart{};
        Options m_options{};

        mutable std::array<uint8_t, TX_BUF_SIZE> m_ring_buffer{};
        mutable std::atomic<size_t> m_head{0};
        mutable std::atomic<size_t> m_tail{0};
        mutable std::atomic<bool> m_is_busy{false};

        auto register_dma_instance() -> void {
            if (m_options.use_dma) {
                s_dma_instance = this;
            }
        }

        static inline UART* s_dma_instance = nullptr;
        friend void ::HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart);

        auto resume_dma_transmission() const -> void {
            if (m_is_busy || m_head == m_tail) return;

            m_is_busy = true;
            size_t const h = m_head;
            size_t const t = m_tail;
            size_t const len = (h > t) ? (h - t) : (TX_BUF_SIZE - t);

            if (HAL_UART_Transmit_DMA(m_huart, &m_ring_buffer[t], static_cast<uint16_t>(len)) == HAL_OK) {
                m_tail = (t + len) % TX_BUF_SIZE;
            } else {
                m_is_busy = false;
            }
        }
    };

#else // HAL_UART_MODULE_ENABLED
    class __attribute__((unavailable("enable 'UART' in STM32CubeMX to use mrover::UART"))) UART {
    public:
        template<typename... Args>
        explicit UART(Args&&... args) {}
    };
#endif // HAL_UART_MODULE_ENABLED

} // namespace mrover
