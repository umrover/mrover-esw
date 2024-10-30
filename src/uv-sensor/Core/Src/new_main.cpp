#include "new_main.h"
#include "hardware_adc.hpp"
#include "UVSensor.hpp"

extern ADC_HandleTypeDef hadc1;

mrover::ADCSensor adc_sensor = mrover::ADCSensor(&hadc1, 1);
mrover::UVSensor uv_sensor = mrover::UVSensor(&adc_sensor, 0);

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	uv_sensor.update_uv_async();
}

void new_main(){
	float x = 0;
	adc_sensor.update();
	while(1){
		x = uv_sensor.get_current_uv();
	}
}
