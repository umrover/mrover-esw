#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#ifdef STM32
#include "main.h"
#endif // STM32

namespace mrover {

#ifdef HAL_ADC_MODULE_ENABLED
    /**
     * Wrapper of ADC peripheral, enabling DMA, self-calibration, and interfacing with analog sensors.
     */
    class ADC {
    public:
        enum class mode_t : uint8_t {
            POLLING,
            DMA,
        };

        struct options_t {
            mode_t mode{mode_t::POLLING};
            bool self_calibrate{true}; // run the offset self-calibration in init()
            uint32_t timeout_ms{10};   // per-conversion timeout
        };

        static constexpr std::size_t MAX_CHANNELS = 16;

        ADC(ADC_HandleTypeDef* hadc, options_t const& options)
            : m_hadc{hadc}, m_options{options} {}

        ADC(ADC const&) = delete;
        auto operator=(ADC const&) -> ADC& = delete;
        ADC(ADC&&) = delete;
        auto operator=(ADC&&) -> ADC& = delete;

        /**
         * Calibrate (if configured) and start ADC in DMA mode (if configured).
         *
         * @return false on invalid setup or failed HAL call
         */
        auto init() -> bool {
            m_channels = std::min<size_t>(m_hadc->Init.NbrOfConversion, MAX_CHANNELS);

            if (m_options.mode == mode_t::POLLING && m_channels > 1 && m_hadc->Init.EOCSelection != ADC_EOC_SINGLE_CONV) return false;
            if (m_options.mode == mode_t::DMA && (m_hadc->DMA_Handle == nullptr || m_hadc->DMA_Handle->Init.MemDataAlignment != DMA_MDATAALIGN_WORD)) return false;

            if (m_options.self_calibrate && HAL_ADCEx_Calibration_Start(m_hadc, ADC_SINGLE_ENDED) != HAL_OK) return false;

            if (m_options.mode == mode_t::DMA)
                return HAL_ADC_Start_DMA(m_hadc, const_cast<uint32_t*>(m_values), static_cast<uint32_t>(m_channels)) == HAL_OK;
            return true;
        }

        /**
         * Sample ADC.
         *
         * Will acquire a new sample if DMA is not configured, or wait until DMA has a new sample.
         *
         * @return if a sample has been successfully sampled
         */
        auto sample() -> bool {
            if (m_options.mode == mode_t::DMA) {
                uint32_t const seen = m_conversions;
                for (uint32_t const start = HAL_GetTick(); m_conversions == seen;) {
                    if (HAL_GetTick() - start > m_options.timeout_ms) return false;
                }
                return true;
            }
            if (HAL_ADC_Start(m_hadc) != HAL_OK) return false;
            bool ok = true;
            for (size_t i = 0; i < m_channels && ok; ++i) {
                ok = HAL_ADC_PollForConversion(m_hadc, m_options.timeout_ms) == HAL_OK;
                if (ok) m_values[i] = HAL_ADC_GetValue(m_hadc);
            }
            HAL_ADC_Stop(m_hadc);
            return ok;
        }

        /**
         * Get latest sample on specified channel.
         *
         * @return sample value
         */
        [[nodiscard]] auto value(size_t const i) const -> uint32_t {
            return i < m_channels ? m_values[i] : 0;
        }

        /**
         * Sequential calls to sample and value.
         *
         * @return most recently sampled value
         */
        auto read(size_t const i) -> uint32_t {
            return sample() ? value(i) : 0;
        }

        /**
         * Call from HAL_ADC_ConvCpltCallback for this ADC.
         */
        auto handle_conversion_complete() -> void {
            m_conversions = m_conversions + 1;
        }

        // DMA: completed sequences since init(), wraps
        [[nodiscard]] auto conversions() const -> uint32_t { return m_conversions; }
        [[nodiscard]] auto channels() const -> size_t { return m_channels; }
        [[nodiscard]] auto handle() const -> ADC_HandleTypeDef* { return m_hadc; }

    private:
        ADC_HandleTypeDef* m_hadc;
        options_t m_options;
        size_t m_channels{};
        uint32_t volatile m_values[MAX_CHANNELS]{};
        uint32_t volatile m_conversions{};
    };
#else  // HAL_ADC_MODULE_ENABLED
    class __attribute__((unavailable("enable 'ADC' in STM32CubeMX to use mrover::ADC"))) ADC {
    public:
        template<typename... Args>
        explicit ADC(Args&&... args) {}
    };
#endif // HAL_ADC_MODULE_ENABLED

} // namespace mrover
