#pragma once

#include "main.h"

class Servo {
public:
	Servo(TIM_HandleTypeDef* tim_handle, uint32_t channel, uint32_t max_deg, uint32_t zero_deg_ccr, uint32_t max_deg_ccr);

	void set_angle(uint32_t angle);

private:
	TIM_HandleTypeDef* m_tim_handle;
	uint32_t m_channel;
	uint32_t m_max_deg;
	uint32_t m_zero_deg_ccr;
	uint32_t m_max_deg_ccr;
	uint32_t m_ccr;
};
