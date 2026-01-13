#include <hw/quadrature.hpp>
#include <util.hpp>

namespace mrover {

#ifdef HAL_TIM_MODULE_ENABLED
    QuadratureEncoderReader::QuadratureEncoderReader(
            TIM_HandleTypeDef* tick_timer,
            ElapsedTimer const& elapsed_timer,
            Ratio const multiplier,
            Ticks const cpr) : m_tick_timer{tick_timer},
                               m_elapsed_timer{elapsed_timer},
                               m_multiplier{multiplier},
                               m_cpr{cpr} {
        m_counts_unwrapped_prev = __HAL_TIM_GET_COUNTER(m_tick_timer);
        check(HAL_TIM_Encoder_Start_IT(m_tick_timer, TIM_CHANNEL_ALL) == HAL_OK, Error_Handler);
    }

    auto count_delta_and_update(std::uint16_t& previous, TIM_HandleTypeDef const* timer) -> std::int16_t {
        auto const now = static_cast<std::uint16_t>(__HAL_TIM_GET_COUNTER(timer));
        auto const delta = static_cast<std::int16_t>(now - previous);
        previous = now;
        return delta;
    }

    [[nodiscard]] auto QuadratureEncoderReader::read() const -> std::optional<QuadratureEncoderReading> {
        return std::make_optional(QuadratureEncoderReading{
                .position = m_position,
                .velocity = m_velocity_filter.get_filtered()});
    }

    auto QuadratureEncoderReader::update() -> void {
        Seconds const elapsed_time = m_elapsed_timer.get_time_since_last_read();
        std::int16_t const delta_ticks = count_delta_and_update(m_counts_unwrapped_prev, m_tick_timer);
        // TODO(eric) fix this monstrosity
        auto const delta_angle = Radians{m_multiplier.get() * static_cast<float>(delta_ticks) / m_cpr.get()};

        m_position += delta_angle;
        m_velocity_filter.add_reading(delta_angle / elapsed_time);
    }
#endif

} // namespace mrover
