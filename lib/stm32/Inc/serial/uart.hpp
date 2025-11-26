#pragma once

#include <cstdint>
#include <string_view>
#include <span>

#include <util.hpp>

#include "main.h"

namespace mrover {

#ifdef HAL_UART_MODULE_ENABLED

    class UART {
    public:
        struct Options {
            Options() {}
            uint32_t timeout_ms{100};
        };

        UART() = default;

        explicit UART(
            UART_HandleTypeDef* huart,
            Options const& options = Options()
        ) :
            m_huart{huart},
            m_options{options}
        {}

        auto transmit(std::string_view const data) const -> void {
            auto* ptr = reinterpret_cast<uint8_t const*>(data.data());

            check(HAL_UART_Transmit(m_huart, const_cast<uint8_t*>(ptr), static_cast<uint16_t>(data.size()), m_options.timeout_ms) == HAL_OK, Error_Handler);
        }

        auto transmit(uint8_t const byte) const -> void {
            check(HAL_UART_Transmit(m_huart, const_cast<uint8_t*>(&byte), 1, m_options.timeout_ms) == HAL_OK, Error_Handler);
        }

        [[nodiscard]] auto receive(std::span<uint8_t> buffer) const -> bool {
            if (buffer.empty()) return true;

            auto const status = HAL_UART_Receive(m_huart, buffer.data(), static_cast<uint16_t>(buffer.size()), m_options.timeout_ms);
            return status == HAL_OK;
        }

        [[nodiscard]] auto receive_byte(uint8_t& out_byte) const -> bool {
            auto const status = HAL_UART_Receive(m_huart, &out_byte, 1, m_options.timeout_ms);
            return status == HAL_OK;
        }

        auto reset() const -> void {
            HAL_UART_Abort(m_huart);
        }

        [[nodiscard]] auto handle() const -> UART_HandleTypeDef* {
            return m_huart;
        }

    private:
        UART_HandleTypeDef* m_huart{};
        Options m_options{};
    };

#else // HAL_UART_MODULE_ENABLED
    class __attribute__((unavailable("enable 'UART' in STM32CubeMX to use mrover::UART"))) UART {
    public:
        template<typename... Args>
        explicit UART(Args&&... args) {}
    };
#endif // HAL_UART_MODULE_ENABLED

} // namespace mrover
