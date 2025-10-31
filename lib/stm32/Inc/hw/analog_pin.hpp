#pragma once

#include <cstdint>
#include "main.h"

namespace mrover {

class AnalogPin {
public:
    AnalogPin() = default;

    AnalogPin(ADC_HandleTypeDef* adc, uint32_t channel)
        : m_adc(adc), m_channel(channel) {}

    [[nodiscard]] auto read_analog() const -> uint16_t {
        ADC_ChannelConfTypeDef sConfig{};
        sConfig.Channel = m_channel;
        sConfig.Rank = ADC_REGULAR_RANK_1;
        sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;

        HAL_ADC_ConfigChannel(m_adc, &sConfig);

        HAL_ADC_Start(m_adc);
        HAL_ADC_PollForConversion(m_adc, HAL_MAX_DELAY);
        uint16_t value = HAL_ADC_GetValue(m_adc);
        HAL_ADC_Stop(m_adc);

        return value; // 0–4095 for 12-bit ADC
    }

private:
    ADC_HandleTypeDef* m_adc{};
    uint32_t m_channel{};
};

} // namespace mrover
