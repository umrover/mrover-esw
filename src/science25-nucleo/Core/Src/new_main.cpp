#include "new_main.h"
#include "hardware_adc.hpp"
#include "UVSensor.hpp"
#include "MethaneSensor.hpp"
#include "OxygenSensor.hpp"
#include "TempHumiditySensor.hpp"

extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;




mrover::ADCSensor adc_sensor = mrover::ADCSensor(&hadc1, 0);
mrover::MethaneSensor methane_sensor = mrover::MethaneSensor(&adc_sensor, 0);
mrover::OxygenSensor oxygen_sensor = mrover::OxygenSensor(&hi2c2);
mrover::TempHumiditySensor th_sensor = mrover::TempHumiditySensor(&hi2c2);

//void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
//
//}

void new_main(){
	float x = 0;
	float m = oxygen_sensor.calibrate_oxygen();
	while(1){
		x = oxygen_sensor.update_oxygen();
	}
}
