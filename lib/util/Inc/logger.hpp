#pragma once

#include <cstdint>
#include <string_view>
#include <cstdio>
#include <cstdarg>
#include <optional>

#include <serial/uart.hpp>

namespace mrover {

    static constexpr size_t LOG_BUFFER_SIZE = 128;

#ifdef HAL_UART_MODULE_ENABLED

    class Logger {
    public:
        enum class log_level_t : uint8_t {
            LOG_DEBUG,
            LOG_INFO,
            LOG_WARNING,
            LOG_ERROR,
            LOG_NONE
        };

        Logger() = delete;

        static void init(UART_HandleTypeDef* huart, log_level_t const level = log_level_t::LOG_INFO) {
            s_huart = huart;
            s_level = level;
        }

        static Logger* get_instance() {
            static Logger singleton{s_huart, s_level};
            return &singleton;
        }

        auto set_level(log_level_t const level) -> void {
            m_level = level;
        }

        auto debug(const char* format, ...) const -> void {
            if (log_level_t::LOG_DEBUG >= m_level) {
                va_list args;
                va_start(args, format);
                vlog(log_level_t::LOG_DEBUG, "DEBUG: ", format, args);
                va_end(args);
            }
        }

        auto info(const char* format, ...) const -> void {
            if (log_level_t::LOG_INFO >= m_level) {
                va_list args;
                va_start(args, format);
                vlog(log_level_t::LOG_INFO, "INFO: ", format, args);
                va_end(args);
            }
        }

        auto warn(const char* format, ...) const -> void {
            if (log_level_t::LOG_WARNING >= m_level) {
                va_list args;
                va_start(args, format);
                vlog(log_level_t::LOG_WARNING, "WARNING: ", format, args);
                va_end(args);
            }
        }

        auto error(const char* format, ...) const -> void {
            if (log_level_t::LOG_ERROR >= m_level) {
                va_list args;
                va_start(args, format);
                vlog(log_level_t::LOG_ERROR, "ERROR: ", format, args);
                va_end(args);
            }
        }

        auto debug(std::string_view const message) const -> void {
            log(log_level_t::LOG_DEBUG, "DEBUG: ", message);
        }

        auto info(std::string_view const message) const -> void {
            log(log_level_t::LOG_INFO, "INFO: ", message);
        }

        auto warn(std::string_view const message) const -> void {
            log(log_level_t::LOG_WARNING, "WARNING: ", message);
        }

        auto error(std::string_view const message) const -> void {
            log(log_level_t::LOG_ERROR, "ERROR: ", message);
        }

    private:
        UART m_uart;
        log_level_t m_level;

        static inline UART_HandleTypeDef* s_huart = nullptr;
        static inline auto s_level = log_level_t::LOG_INFO;

        explicit Logger(UART_HandleTypeDef* huart, log_level_t const level) : m_uart{huart}, m_level{level} {}

        auto vlog(log_level_t const level, std::string_view const prefix,
                  const char* format, va_list const args) const -> void {
            if (level < m_level) return;

            static char buffer[LOG_BUFFER_SIZE];
            if (auto const len = std::vsnprintf(buffer, LOG_BUFFER_SIZE, format, args); len > 0 && static_cast<size_t>(len) < LOG_BUFFER_SIZE) {
                m_uart.transmit(prefix);
                m_uart.transmit(std::string_view(buffer, len));
                m_uart.transmit("\r\n");
            } else {
                m_uart.transmit("LOGGER: format buffer error.\r\n");
            }
        }

        auto log(log_level_t const level, std::string_view const prefix, std::string_view const message) const -> void {
            if (level < m_level) return;
            m_uart.transmit(prefix);
            m_uart.transmit(message);
            m_uart.transmit("\r\n");
        }
    };

#else // HAL_UART_MODULE_ENABLED
    class __attribute__((unavailable("enable 'UART' in STM32CubeMX to use mrover::Logger"))) Logger {
    public:
        template<typename... Args>
        explicit Logger(Args&&... args) {}
    };
#endif // HAL_UART_MODULE_ENABLED

} // namespace mrover
