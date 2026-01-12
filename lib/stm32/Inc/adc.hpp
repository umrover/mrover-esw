#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <vector>

#include "main.h"

namespace mrover {

#ifdef HAL_ADC_MODULE_ENABLED
    class ADC {
    public:
        ADC() = default;

        ADC(ADC_HandleTypeDef* hadc, uint8_t const channels)
            : m_hadc(hadc), m_channels(channels) {
            m_values.resize(channels);
        }

        uint16_t get_raw_channel_value(uint8_t channel) {
            return m_values.at(channel);
        }

        uint16_t get_raw_channel_blocking() {
            if (HAL_ADC_Start(m_hadc) != HAL_OK) {
                Error_Handler();
            }

            if (HAL_ADC_PollForConversion(m_hadc, HAL_MAX_DELAY) == HAL_OK) {
                return HAL_ADC_GetValue(m_hadc);
            }

            return 0;
        }

        void update() {
            HAL_ADC_Start_DMA(m_hadc, reinterpret_cast<uint32_t*>(m_values.data()), m_channels);
        }

    private:
        ADC_HandleTypeDef* m_hadc;
        uint8_t m_channels;
        std::vector<uint32_t> m_values;
    };
#else // HAL_ADC_MODULE_ENABLED
    class __attribute__((unavailable("enable 'ADC' in STM32CubeMX to use mrover::ADC"))) ADC {
    public:
        template<typename... Args>
        explicit ADC(Args&&... args) {}
    };
#endif // HAL_ADC_MODULE_ENABLED

} // namespace mrover
