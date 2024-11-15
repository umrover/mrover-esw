#include "new_main.h"
#include "hardware_adc.hpp"
#include "UVSensor.hpp"
#include "MethaneSensor.hpp"
#include "OxygenSensor.hpp"

extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c2;

#define ADDRESS_3 0x73
#define OXYGEN_DATA_REGISTER 0x03
#define OXYGEN_DATA_LENGTH 8

//mrover::ADCSensor adc_sensor = mrover::ADCSensor(&hadc1, 0);
uint8_t buf[3];
mrover::OxygenSensor oxygen_sensor = mrover::OxygenSensor(ADDRESS_3, OXYGEN_DATA_REGISTER, buf, OXYGEN_DATA_LENGTH, &hi2c2);

//void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
//
//}

void new_main(){
	float x = 0;
	while(1){
		x = oxygen_sensor.update_oxygen();
	}
}
