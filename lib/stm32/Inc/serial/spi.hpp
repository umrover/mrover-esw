#pragma once

#include <cstdint>
#include <functional>
#include <span>

#include <util.hpp>

#ifdef STM32
#include "main.h"
#endif // STM32

namespace mrover {

#ifdef HAL_SPI_MODULE_ENABLED

    class SPI {
    public:
        static constexpr size_t QUEUE_SIZE = 16;

        struct options_t {
            options_t() {}
            uint32_t timeout_ms{10};
            bool use_dma{false};
        };

        // NOTE: data width in must be set to 'Half Word' (16-bit)
        struct transaction_t {
            std::span<uint16_t> tx;
            std::span<uint16_t> rx;
            std::function<void()> callback;
        };

        SPI() = default;
        explicit SPI(SPI_HandleTypeDef* hspi, options_t const& options = options_t()) : m_hspi{hspi}, m_options{options} {
            if (m_options.use_dma) s_dma_instance = this;
        }

        auto transfer(std::span<uint16_t> tx, std::span<uint16_t> rx, std::function<void()> const& callback = nullptr) -> void {
            if (tx.size() != rx.size()) return;

            if (!m_options.use_dma) {
                auto const status = HAL_SPI_TransmitReceive(m_hspi, reinterpret_cast<uint8_t*>(tx.data()), reinterpret_cast<uint8_t*>(rx.data()), static_cast<uint16_t>(tx.size()), m_options.timeout_ms);
                check(status == HAL_OK, Error_Handler);
                if (callback) callback();
                return;
            }

            // TODO(eric): error here
            if (m_queue_count >= QUEUE_SIZE) return;

            m_queue[m_head] = {tx, rx, callback};
            m_head = (m_head + 1) % QUEUE_SIZE;
            m_queue_count++;

            resume_dma_transmission();
        }

        auto handle_irq() -> void {
            m_is_busy = false;
            if (m_queue[m_tail].callback) m_queue[m_tail].callback();

            m_tail = (m_tail + 1) % QUEUE_SIZE;
            m_queue_count--;
            resume_dma_transmission();
        }

        [[nodiscard]] auto is_busy() const -> bool { return m_is_busy; }
        [[nodiscard]] auto handle() const -> SPI_HandleTypeDef* { return m_hspi; }

        static inline SPI* s_dma_instance = nullptr;

    private:
        SPI_HandleTypeDef* m_hspi{nullptr};
        options_t m_options{};

        transaction_t m_queue[QUEUE_SIZE];
        size_t m_head{0}, m_tail{0}, m_queue_count{0};
        bool volatile m_is_busy{false};

        auto resume_dma_transmission() -> void {
            if (m_is_busy || m_queue_count == 0) return;

            m_is_busy = true;
            auto& t = m_queue[m_tail];

            // NOTE: data width in must be set to 'Half Word' (16-bit)
            HAL_SPI_TransmitReceive_DMA(m_hspi, reinterpret_cast<uint8_t*>(t.tx.data()), reinterpret_cast<uint8_t*>(t.rx.data()), static_cast<uint16_t>(t.tx.size()));
        }
    };

#else  // HAL_SPI_MODULE_ENABLED
    class __attribute__((unavailable("enable 'SPI' in STM32CubeMX to use mrover::SPI"))) SPI {
    public:
        template<typename... Args>
        explicit SPI(Args&&... args) {}
    };
#endif // HAL_SPI_MODULE_ENABLED
} // namespace mrover
