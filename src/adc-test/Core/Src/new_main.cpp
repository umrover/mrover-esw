/*
 * LAB Name: STM32 ADC Multi-Channel (Scan) Single-Conversion Mode - DMA
 * Author: Khaled Magdy
 * For More Info Visit: www.DeepBlueMbedded.com
*/
#include "main.h"
#include "adc.hpp"
#include "stdio.h"

extern ADC_HandleTypeDef hadc1;

namespace mrover {
ADCSensor adc_sensor;
uint32_t adc_buf[8];

void event_loop() {
	while (true) {}
}

void init() {
	adc_sensor = ADCSensor(&hadc1, 8);
	adc_sensor.start_dma();

	event_loop();
}
}

extern "C" {

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	if (hadc->Instance == ADC1) {
		for (int i = 0; i < 8; i++) {
			mrover::adc_buf[i] = mrover::adc_sensor.get_raw_channel_value(i);
		}
	}
}

void HAL_PostInit() {
	mrover::init();
}

}
