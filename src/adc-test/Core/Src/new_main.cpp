/*
 * LAB Name: STM32 ADC Multi-Channel (Scan) Single-Conversion Mode - DMA
 * Author: Khaled Magdy
 * For More Info Visit: www.DeepBlueMbedded.com
*/
#include "main.h"
#include "stdio.h"

uint32_t adc_buf[8];
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

int init() {
	HAL_ADC_Start_DMA(&hadc1, adc_buf, 8);

	while (true) {}
}

extern "C" {

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
//	if (hadc->Instance == ADC1) {
//		for (int i = 0; i < 8; i++) {
//			printf("channel %d: %d\r\n", i, adc_buf[i]);
//		}
//	}
}

void HAL_PostInit() {
	init();
}

}
