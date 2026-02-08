#include "adc.hpp"
#include "main.h"
#include <cstdint>

namespace mrover {
    class UVSensor {
    private:
        float uv_index{0.0};
        uint16_t adc_res{4095};

    public:
        ADC adc;

        // default constructor
        UVSensor() = default;

        // adc_in is a pointer to the desired ADCSensor and channel_in is the channel that the UVsensor is on
        UVSensor(ADC_HandleTypeDef* adc_in)
            : adc(adc_in, 1) {}

        // sample the sensor by starting a dma transaction
        void sample_sensor() {
            adc.start_dma();
        }

        // update value of uv_index
        float update_uv() {
            uv_index = 33.0 * ((float)adc.get_raw_channel_value(0) / (float)adc_res);
            return uv_index;
        }

        // returns the current value of uv_index
        float get_current_uv() {
            return uv_index;
        }
    }; // class UVSensor
} // namespace mrover