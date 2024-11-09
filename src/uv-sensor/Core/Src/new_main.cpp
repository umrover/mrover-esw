#include "new_main.h"
#include "hardware_adc.hpp"
#include "UVSensor.hpp"
#include "MethaneSensor.hpp"

extern ADC_HandleTypeDef hadc1;

mrover::ADCSensor adc_sensor = mrover::ADCSensor(&hadc1, 1);
mrover::MethaneSensor methane_sensor = mrover::MethaneSensor(&adc_sensor, 0);

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	methane_sensor.update_ppm_async();
}

void new_main(){
	float x = 0;
	while(1){
		x = methane_sensor.update_ppm_blocking();
	}
}
