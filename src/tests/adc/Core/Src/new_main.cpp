#include <adc.hpp>

extern ADC_HandleTypeDef hadc1;

namespace mrover {
    ADC adc_sensor{&hadc1, ADC::options_t{.mode = ADC::mode_t::DMA}};
    uint32_t adc_buf[8];

    void event_loop() {
        while (true) {}
    }

    void init() {
        adc_sensor.init();

        event_loop();
    }
} // namespace mrover

extern "C" {

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        for (int i = 0; i < 8; i++) {
            mrover::adc_buf[i] = mrover::adc_sensor.value(i);
        }
    }
}

void PostInit() {
    mrover::init();
}
}
