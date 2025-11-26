#pragma once

#include <units.hpp>
#include <filtering.hpp>
#include <timer.hpp>

#include "main.h"

namespace mrover {

    struct QuadratureEncoderReading {
        Radians position;
        RadiansPerSecond velocity;
    };

#ifdef HAL_TIM_MODULE_ENABLED
    /**
     * 2-phase quadrature encoder implementation
     *
     * ensure STM32 is configured to read encoder mode on the specified timer
     */
    class QuadratureEncoderReader {
    public:
        QuadratureEncoderReader() = default;

        QuadratureEncoderReader(
            TIM_HandleTypeDef* tick_timer,
            const ElapsedTimer& elapsed_timer,
            Ratio multiplier,
            Ticks cpr
        );

        [[nodiscard]] auto read() const -> std::optional<QuadratureEncoderReading>;

        auto update() -> void;

        auto expired() -> void {
            m_velocity_filter.add_reading(RadiansPerSecond{0});
        }

    private:
        static constexpr std::size_t VELOCITY_BUFFER_SIZE = 16;

        TIM_HandleTypeDef* m_tick_timer{};
        ElapsedTimer m_elapsed_timer;

        std::uint16_t m_counts_unwrapped_prev{};
        Ratio m_multiplier;
        Ticks m_cpr;

        Radians m_position;
        RunningMeanFilter<RadiansPerSecond, VELOCITY_BUFFER_SIZE> m_velocity_filter;
    };
#else // HAL_TIM_MODULE_ENABLED
    class __attribute__((unavailable("enable 'TIM' in STM32CubeMX to use mrover::QuadratureEncoderReader"))) QuadratureEncoderReader {
        public:
        template<typename... Args>
        explicit QuadratureEncoderReader(Args&&... args) {}
    };
#endif // HAL_TIM_MODULE_ENABLED

} // namespace mrover
