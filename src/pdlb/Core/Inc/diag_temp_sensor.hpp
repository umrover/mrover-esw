#pragma once

#include "hardware_adc.hpp"
#include "stm32g4xx_hal.h"

#include <math.h>
#include <memory>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// TMP6431DECR

namespace mrover {

    class DiagTempSensor {
    public:
        DiagTempSensor() = default;

        DiagTempSensor(std::shared_ptr<ADCSensor> adc_sensor, uint8_t channel);

        void update_temp();

        float get_temp();

    private:
        std::shared_ptr<ADCSensor> m_adc_sensor;
        uint8_t m_channel; // channel of adc sensor

        float m_temp{};
    };

} // namespace mrover
