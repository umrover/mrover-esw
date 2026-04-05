#pragma once

#include <cstdint>
#include <span>

#include <util.hpp>

#ifdef STM32
#include "main.h"
#endif // STM32

namespace mrover {

#ifdef HAL_SPI_MODULE_ENABLED

    class SPI {
    public:
        struct options_t {
            options_t() {}
            uint32_t timeout_ms{10};
        };

        SPI() = default;
        explicit SPI(SPI_HandleTypeDef* hspi, options_t const& options = options_t())
            : m_hspi{hspi}, m_options{options} {}

        // Simple blocking transfer using spans
        auto transfer(std::span<uint16_t> tx, std::span<uint16_t> rx) -> bool {
            if (tx.size() != rx.size()) return false;

            auto const status = HAL_SPI_TransmitReceive(
                m_hspi,
                reinterpret_cast<uint8_t*>(tx.data()),
                reinterpret_cast<uint8_t*>(rx.data()),
                static_cast<uint16_t>(tx.size()),
                m_options.timeout_ms
            );

            check(status == HAL_OK, Error_Handler);
            return status == HAL_OK;
        }

        [[nodiscard]] auto handle() const -> SPI_HandleTypeDef* { return m_hspi; }

    private:
        SPI_HandleTypeDef* m_hspi{nullptr};
        options_t m_options{};
    };

#else  // HAL_SPI_MODULE_ENABLED
    class __attribute__((unavailable("enable 'SPI' in STM32CubeMX to use mrover::SPI"))) SPI {
        public:
        template<typename... Args>
        explicit SPI(Args&&... args) {}
    };
#endif // HAL_SPI_MODULE_ENABLED
} // namespace mrover