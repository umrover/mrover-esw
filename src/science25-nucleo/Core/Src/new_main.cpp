#include "new_main.h"
#include "hardware_adc.hpp"
#include "UVSensor.hpp"
#include "MethaneSensor.hpp"
#include "OxygenSensor.hpp"
#include "TempHumiditySensor.hpp"

extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;

#define ADDRESS_3 0x73
#define OXYGEN_DATA_REGISTER 0x03
#define OXYGEN_DATA_LENGTH 8
#define TEMP_HUM_ADDRESS 0x38
#define TEMP_HUM_SIZE 6
uint8_t buf[3];
uint8_t buf_two[6];
mrover::ADCSensor adc_sensor = mrover::ADCSensor(&hadc1, 0);
mrover::MethaneSensor methane_sensor = mrover::MethaneSensor(&adc_sensor, 0);
mrover::OxygenSensor oxygen_sensor = mrover::OxygenSensor(ADDRESS_3, OXYGEN_DATA_REGISTER, buf, OXYGEN_DATA_LENGTH, &hi2c2);
mrover::TempHumiditySensor th_sensor = mrover::TempHumiditySensor(TEMP_HUM_ADDRESS, buf_two, TEMP_HUM_SIZE, &hi2c2);

//void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
//
//}

void new_main(){
	float x = 0;
	while(1){
		th_sensor.update_temp_humidity();
		x = th_sensor.get_current_temp();
		x = th_sensor.get_current_humidity();
	}
}
