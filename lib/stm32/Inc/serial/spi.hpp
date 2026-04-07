#pragma once

#include <functional>
#include <span>

#include <hw/pin.hpp>
#include <sys.hpp>

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
            uint32_t timeout_ms{HAL_MAX_DELAY};
            bool use_dma{false};
        };

        struct transaction_t {
            std::span<uint16_t> tx;
            std::span<uint16_t> rx;
            std::function<void()> callback;
            Pin* cs_pin{nullptr};
        };

        SPI() = default;
        explicit SPI(SPI_HandleTypeDef* hspi, options_t const& options = options_t())
            : m_hspi{hspi}, m_options{options} {
            if (m_options.use_dma) s_dma_instance = this;
        }

        auto transfer(std::span<uint16_t> tx, std::span<uint16_t> rx, std::function<void()> const& callback = nullptr, Pin* cs_pin = nullptr) -> bool {
            if (tx.size() != rx.size()) return false;

            if (!m_options.use_dma) {
                if (cs_pin) cs_pin->reset();
                auto const status = HAL_SPI_TransmitReceive(
                        m_hspi,
                        reinterpret_cast<uint8_t*>(tx.data()),
                        reinterpret_cast<uint8_t*>(rx.data()),
                        static_cast<uint16_t>(tx.size()),
                        m_options.timeout_ms);
                if (cs_pin) cs_pin->set();

                if (status == HAL_OK && callback) callback();
                return status == HAL_OK;
            }

            // dma enabled
            // TODO(eric): something other than dropping messages?
            if (m_queue_count >= QUEUE_SIZE) return false;

            {
                System::InterruptGuard();
                m_queue[m_head] = {tx, rx, callback, cs_pin};
                m_head = (m_head + 1) % QUEUE_SIZE;
                m_queue_count += 1;
            }

            resume_dma_transmission();
            return true;
        }

        auto handle_irq() -> void {
            m_is_busy = false;
            auto const& t = m_queue[m_tail];

            if (t.cs_pin) t.cs_pin->set();
            if (t.callback) t.callback();
            m_tail = (m_tail + 1) % QUEUE_SIZE;
            m_queue_count -= 1;

            resume_dma_transmission();
        }

        [[nodiscard]] auto handle() const -> SPI_HandleTypeDef* { return m_hspi; }
        [[nodiscard]] auto is_busy() const -> bool { return m_is_busy; }

        static inline SPI* s_dma_instance = nullptr;

    private:
        SPI_HandleTypeDef* m_hspi{nullptr};
        options_t m_options{};

        transaction_t m_queue[QUEUE_SIZE];
        size_t m_head{0}, m_tail{0};
        size_t volatile m_queue_count{0};
        bool volatile m_is_busy{false};

        auto resume_dma_transmission() -> void {
            if (m_is_busy || m_queue_count == 0) return;

            m_is_busy = true;
            auto& t = m_queue[m_tail];

            if (t.cs_pin) t.cs_pin->reset();

            HAL_SPI_TransmitReceive_DMA(
                    m_hspi,
                    reinterpret_cast<uint8_t*>(t.tx.data()),
                    reinterpret_cast<uint8_t*>(t.rx.data()),
                    static_cast<uint16_t>(t.tx.size()));
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
