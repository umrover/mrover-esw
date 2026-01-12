#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>

#include <units.hpp>
#include <logger.hpp>

#include "main.h"

namespace mrover {

#ifdef HAL_TIM_MODULE_ENABLED
    class Timer {
        TIM_HandleTypeDef* htim;
        uint32_t sys_clock;
        std::string name;
        bool interrupt;
        bool is_en = false;

        auto start_no_it() const -> HAL_StatusTypeDef {
            return HAL_TIM_Base_Start(htim);
        }

        auto start_it() const -> HAL_StatusTypeDef {
            return HAL_TIM_Base_Start_IT(htim);
        }

        auto stop_no_it() const -> HAL_StatusTypeDef {
            return HAL_TIM_Base_Stop(htim);
        }

        auto stop_it() const -> HAL_StatusTypeDef {
            return HAL_TIM_Base_Stop_IT(htim);
        }

    public:
        Timer() = default;
        explicit Timer(TIM_HandleTypeDef* tim_handle, bool const interrupt = false, std::string const& name = "Timer")
            : htim{tim_handle}, sys_clock{HAL_RCC_GetSysClockFreq()}, name{std::move(name)}, interrupt{interrupt} {
            start();
        }

        auto start() -> void {
            if (interrupt) {
                check(start_it() == HAL_OK, Error_Handler);
                is_en = true;
            } else {
                check(start_no_it() == HAL_OK, Error_Handler);
                is_en = true;
            }
            Logger::instance().info("%s @%.2f Hz", name.c_str(), get_update_frequency().rep);
        }

        auto stop() -> void {
            if (interrupt) {
                check(stop_it() == HAL_OK, Error_Handler);
                is_en = false;
            } else {
                check(stop_no_it() == HAL_OK, Error_Handler);
                is_en = false;
            }
        }

        auto is_enabled() const -> bool {
            return is_en;
        }

        auto get_update_frequency() const -> Hertz {
            uint32_t const psc = htim->Instance->PSC;
            uint32_t const arr = htim->Instance->ARR;

            return Hertz{static_cast<float>(sys_clock) / ((psc + 1) * (arr + 1))};
        }

        auto get_counter_frequency() const -> Hertz {
            return Hertz{static_cast<float>(sys_clock) / (htim->Instance->PSC + 1)};
        }

        auto reset() const -> void {
            __HAL_TIM_SetCounter(htim, 0);
        }

    };
#else // HAL_TIM_MODULE_ENABLED
    class __attribute__((unavailable("enable 'TIM' in STM32CubeMX to use mrover::Timer"))) Timer {
        public:
        template<typename... Args>
        explicit Timer(Args&&... args) {}
    };
#endif // HAL_TIM_MODULE_ENABLED

    struct TimerConfig {
        std::uint16_t psc;
        std::uint16_t arr;
    };

    constexpr auto configure_timer_16bit(Hertz const tim_frequency, std::chrono::nanoseconds const period) -> TimerConfig {
        uint32_t const expiration_ticks = (tim_frequency.get() * period.count()) / std::nano::den;

        for (uint16_t psc = 1; psc <= 65535; ++psc) {
            if (uint32_t const arr = expiration_ticks / psc; arr > 0 && (arr - 1) <= 65535) {
                return {static_cast<uint16_t>(psc - 1),
                        static_cast<uint16_t>(arr - 1)};
            }
        }

        return {std::numeric_limits<uint16_t>::max(), std::numeric_limits<uint16_t>::max()};
    }

    class IStopwatch {
    public:
        virtual ~IStopwatch() = default;
        using TimerCallback = std::function<void()>;

        virtual auto remove_stopwatch() -> std::uint8_t = 0;
        virtual auto add_stopwatch() -> std::uint8_t = 0;
        virtual auto add_stopwatch(std::optional<TimerCallback> callback) -> std::uint8_t = 0;
        virtual auto get_time_since_last_read(std::uint8_t index) -> Seconds = 0;
    };

#ifdef HAL_TIM_MODULE_ENABLED
    class ElapsedTimer {
    public:
        // assumes the timer is started outside the scope of this class
        ElapsedTimer() = default;
        ElapsedTimer(TIM_HandleTypeDef* htim, Hertz const frequency) : m_tim(htim), m_period(1.0f / frequency) {};

        auto get_time_since_last_read() -> Seconds {
            Seconds result;
            std::uint32_t const current_tick = __HAL_TIM_GET_COUNTER(m_tim);
            if (m_is_first_read) {
                m_is_first_read = false;
                result = Seconds{0.0f};
            } else {
                result = m_period * (current_tick - m_tick_prev);
            }
            m_tick_prev = current_tick;
            return result;
        }

        auto make_next_read_first_read() -> void {
            m_is_first_read = true;
        }

    private:
        TIM_HandleTypeDef* m_tim{};
        Seconds m_period{};

        bool m_is_first_read = true;

        std::uint32_t m_tick_prev{};
    };
#else // HAL_TIM_MODULE_ENABLED
    class __attribute__((unavailable("enable 'TIM' in STM32CubeMX to use mrover::ElapsedTimer"))) ElapsedTimer {
    public:
        template<typename... Args>
        explicit ElapsedTimer(Args&&... args) {}
    };
#endif // HAL_TIM_MODULE_ENABLED


#ifdef HAL_TIM_MODULE_ENABLED
    template<std::uint8_t MaxStopwatchCount, typename CountType, Hertz TimFrequency>
    class VirtualStopwatches final : public IStopwatch {
        static_assert(std::is_unsigned_v<CountType>, "Template parameter CountType must be an unsigned integer type");
        static_assert(sizeof(CountType) <= 4, "Template parameter CountType must be less than or equal to 32 bits");

    public:
        struct tim_stamp_t {
            CountType last_count{};
            std::size_t num_elapses{};
        };

        struct stopwatch_data_t {
            tim_stamp_t tim_stamp{};
            std::optional<TimerCallback> elapsed_callback{};
        };

        VirtualStopwatches() = default;

        explicit VirtualStopwatches(TIM_HandleTypeDef* htim) : m_hardware_tim(htim) {}

        [[nodiscard]] auto size() const -> std::uint8_t {
            return m_num_stopwatches;
        }

        [[nodiscard]] static auto capacity() -> std::uint8_t {
            return MaxStopwatchCount;
        }

        [[nodiscard]] auto at_capacity() const -> bool {
            return m_num_stopwatches == MaxStopwatchCount;
        }

        auto remove_stopwatch() -> std::uint8_t override {
            return 0;
        }

        auto add_stopwatch() -> std::uint8_t override {
            return add_stopwatch(std::nullopt);
        }

        auto add_stopwatch(std::optional<TimerCallback> callback) -> std::uint8_t override {
            if (m_num_stopwatches >= MaxStopwatchCount) {
                return 0;
            }
            m_stopwatches[m_num_stopwatches].tim_stamp.last_count = get_current_count();
            m_stopwatches[m_num_stopwatches].elapsed_callback = callback;

            return m_num_stopwatches++;
        }

        auto init() const -> void {
            HAL_TIM_Base_Start_IT(m_hardware_tim);
        }

        auto period_elapsed() -> void {
            for (std::size_t i = 0; i < m_num_stopwatches; ++i) {
                ++m_stopwatches[i].tim_stamp.num_elapses;
                if (m_stopwatches[i].elapsed_callback.has_value()) {
                    m_stopwatches[i].elapsed_callback.value()();
                }
            }
        }

        auto get_time_since_last_read(std::uint8_t index) -> Seconds override {
            if (index >= m_num_stopwatches) {
                return static_cast<Seconds>(0);
            }

            HAL_TIM_Base_Stop_IT(m_hardware_tim);
            tim_stamp_t last_tim_stamp = std::exchange(m_stopwatches[index].tim_stamp, {get_current_count(), 0});
            HAL_TIM_Base_Start_IT(m_hardware_tim);


            CountType current_count = m_stopwatches[index].tim_stamp.last_count;

            CountType last_count = last_tim_stamp.last_count;
            std::size_t const num_elapses = last_tim_stamp.num_elapses;

            uint64_t elapsed_counts = 0;

            if (current_count >= last_count) {
                elapsed_counts = static_cast<uint64_t>(current_count) - static_cast<uint64_t>(last_count);
            } else {
                elapsed_counts = (static_cast<uint64_t>(std::numeric_limits<CountType>::max()) - static_cast<uint64_t>(last_count) + 1) + static_cast<uint64_t>(current_count);
            }

            if (num_elapses > 0) {
                uint64_t const counts_per_overflow = static_cast<uint64_t>(std::numeric_limits<CountType>::max()) + 1;
                elapsed_counts += num_elapses * counts_per_overflow;
            }

            return 1 / TimFrequency * elapsed_counts;
        }

    private:
        TIM_HandleTypeDef* m_hardware_tim{};
        std::uint8_t m_num_stopwatches{};
        std::array<stopwatch_data_t, MaxStopwatchCount> m_stopwatches{};

        [[nodiscard]] auto get_current_count() const -> CountType {
            return __HAL_TIM_GetCounter(m_hardware_tim);
        }
    };
#else // HAL_TIM_MODULE_ENABLED
    class __attribute__((unavailable("enable 'TIM' in STM32CubeMX to use mrover::VirtualStopwatches"))) VirtualStopwatches {
    public:
        template<typename... Args>
        explicit VirtualStopwatches(Args&&... args) {}
    };
#endif // HAL_TIM_MODULE_ENABLED

} // namespace mrover
