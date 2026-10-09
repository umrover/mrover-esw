#pragma once

#include <atomic>
#include <cstdint>
#include <span>
#include <string_view>

#include <util.hpp>

#ifdef STM32
#include "main.h"
#endif // STM32

namespace mrover {

#ifdef HAL_UART_MODULE_ENABLED

    /**
     * UART abstraction class
     *
     * Transmit and receive are each synchronous (blocking) or asynchronous (background), chosen independently
     *
     * async transmit requires UART global interrupt, and `handle_tx_complete()` called from `HAL_UART_TxCpltCallback`
     *
     * async receive requires UART global interrupt, and `handle_rx_complete()` /
     * `handle_error()` called from `HAL_UART_RxCpltCallback` / `HAL_UART_ErrorCallback`
     */
    class UART {
    public:
        static constexpr size_t TX_BUF_SIZE = 1024;
        static constexpr size_t RX_BUF_SIZE = 256;

        struct Options {
            Options() {}
            uint32_t timeout_ms{100};     // blocking transmit/receive timeout
            bool use_tx_interrupt{false}; // transmit: true = queue outgoing bytes and send them in the background
            bool use_rx_interrupt{false}; // receive: true = buffer incoming bytes in the background
        };

        UART() = default;

        explicit UART(
                UART_HandleTypeDef* huart,
                Options const& options = Options()) : m_huart{huart},
                                                      m_options{options} {}

        UART(const UART&) = delete;
        UART& operator=(const UART&) = delete;

        UART(UART&& other) noexcept { *this = std::move(other); }
        UART& operator=(UART&& other) noexcept {
            if (this != &other) {
                m_huart = other.m_huart;
                m_options = other.m_options;
                m_tx_head.store(other.m_tx_head.load());
                m_tx_tail.store(other.m_tx_tail.load());
                m_tx_len = other.m_tx_len;
                m_tx_busy = other.m_tx_busy;
                m_rx_head.store(other.m_rx_head.load());
                m_rx_tail = other.m_rx_tail;
                m_rx_started = other.m_rx_started;
            }
            return *this;
        }

        /**
         * Transmit provided serial data
         *
         * Synchronous: waits up to the timeout
         * Asynchronous: pushes to background buffer
         *
         * @param data Data to be sent on wire
         */
        auto transmit(std::string_view const data) -> void {
            if (!m_options.use_tx_interrupt) {
                auto* ptr = reinterpret_cast<uint8_t const*>(data.data());
                HAL_UART_Transmit(m_huart, const_cast<uint8_t*>(ptr), static_cast<uint16_t>(data.size()), m_options.timeout_ms);
                return;
            }

            size_t const tail = m_tx_tail.load(std::memory_order_acquire);
            size_t head = m_tx_head.load(std::memory_order_relaxed);
            for (char const c: data) {
                size_t const next = (head + 1) % TX_BUF_SIZE;
                if (next == tail) break; // drop the rest when buffer is full
                m_tx_buffer[head] = static_cast<uint8_t>(c);
                head = next;
            }
            m_tx_head.store(head, std::memory_order_release);

            // start sending unless a transfer is in flight (its completion sends the rest)
            uint32_t const primask = __get_PRIMASK();
            __disable_irq();
            if (!m_tx_busy) arm_tx_interrupt();
            __set_PRIMASK(primask);
        }

        /**
         * Transmit single byte of serial data
         *
         * @param byte Byte to be sent on wire
         */
        auto transmit(uint8_t const byte) -> void {
            transmit(std::string_view(reinterpret_cast<char const*>(&byte), 1));
        }

        /**
         * Free the bytes just sent in the background and send the next ones
         *
         * This function is called from `HAL_UART_TxCpltCallback` when `use_tx_interrupt` is set
         */
        auto handle_tx_complete() -> void {
            m_tx_tail.store((m_tx_tail.load(std::memory_order_relaxed) + m_tx_len) % TX_BUF_SIZE, std::memory_order_release);
            m_tx_busy = false;
            arm_tx_interrupt();
        }

        /**
         * Receive bytes into buffer
         *
         * Synchronous: waits up to the timeout
         * Asynchronous: pulls from background buffer
         *
         * @param buffer Destination buffer for received bytes
         * @return true if the buffer was filled
         */
        [[nodiscard]] auto receive(std::span<uint8_t> buffer) -> bool {
            if (buffer.empty()) return true;

            if (!m_options.use_rx_interrupt) {
                auto const status = HAL_UART_Receive(m_huart, buffer.data(), static_cast<uint16_t>(buffer.size()), m_options.timeout_ms);
                return status == HAL_OK;
            }

            if (!m_rx_started) {
                m_rx_started = true;
                arm_rx_interrupt();
            }
            size_t const head = m_rx_head.load(std::memory_order_acquire);
            if ((head + RX_BUF_SIZE - m_rx_tail) % RX_BUF_SIZE < buffer.size()) return false;
            for (uint8_t& byte: buffer) {
                byte = m_rx_buffer[m_rx_tail];
                m_rx_tail = (m_rx_tail + 1) % RX_BUF_SIZE;
            }
            return true;
        }

        /**
         * Receive single byte
         *
         * @param out_byte Destination for the received byte
         * @return true if a byte was received
         */
        [[nodiscard]] auto receive_byte(uint8_t& out_byte) -> bool {
            return receive({&out_byte, 1});
        }

        /**
         * Store a byte received in the background and wait for the next one
         *
         * This function is called from `HAL_UART_RxCpltCallback` when `use_rx_interrupt` is set
         */
        auto handle_rx_complete() -> void {
            size_t const head = m_rx_head.load(std::memory_order_relaxed);
            size_t const next = (head + 1) % RX_BUF_SIZE;
            if (next != m_rx_tail) { // drop byte when buffer is full
                m_rx_buffer[head] = m_rx_byte;
                m_rx_head.store(next, std::memory_order_release);
            }
            arm_rx_interrupt();
        }

        /**
         * Restart background reception, which HAL stops on overrun/framing errors
         *
         * This function is called from `HAL_UART_ErrorCallback` when `use_rx_interrupt` is set
         */
        auto handle_error() -> void {
            if (m_rx_started) arm_rx_interrupt();
        }

        /**
         * Reset the UART peripheral (abort current transaction)
         */
        auto reset() const -> void {
            HAL_UART_Abort(m_huart);
        }

        /**
         * Get the HAL UART handle under the instance
         *
         * @return The underlying UART handle
         */
        [[nodiscard]] auto handle() const -> UART_HandleTypeDef* {
            return m_huart;
        }

    private:
        UART_HandleTypeDef* m_huart{};
        Options m_options{};

        std::array<uint8_t, TX_BUF_SIZE> m_tx_buffer{};
        std::atomic<size_t> m_tx_head{0}; // written by transmit()
        std::atomic<size_t> m_tx_tail{0}; // written by the transmit interrupt
        size_t m_tx_len{0};               // bytes in the transfer in flight
        bool m_tx_busy{false};

        std::array<uint8_t, RX_BUF_SIZE> m_rx_buffer{};
        std::atomic<size_t> m_rx_head{0}; // written by the receive interrupt
        size_t m_rx_tail{0};              // written by receive()
        uint8_t m_rx_byte{};
        bool m_rx_started{false};

        auto arm_rx_interrupt() -> void {
            HAL_UART_Receive_IT(m_huart, &m_rx_byte, 1);
        }

        // send the next contiguous run of queued bytes; called with the transmit interrupt unable to run
        auto arm_tx_interrupt() -> void {
            size_t const head = m_tx_head.load(std::memory_order_acquire);
            size_t const tail = m_tx_tail.load(std::memory_order_relaxed);
            if (head == tail) return;
            m_tx_len = head > tail ? head - tail : TX_BUF_SIZE - tail;
            m_tx_busy = HAL_UART_Transmit_IT(m_huart, &m_tx_buffer[tail], static_cast<uint16_t>(m_tx_len)) == HAL_OK;
        }
    };

#else  // HAL_UART_MODULE_ENABLED
    class __attribute__((unavailable("enable 'UART' in STM32CubeMX to use mrover::UART"))) UART {
    public:
        template<typename... Args>
        explicit UART(Args&&... args) {}
    };
#endif // HAL_UART_MODULE_ENABLED

} // namespace mrover
