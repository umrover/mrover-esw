#include "servo.hpp"

// TODO: Implement the Servo constructor
Servo::Servo(TIM_HandleTypeDef* tim_handle, uint32_t channel) {
	m_tim_handle = tim_handle;
	m_channel = channel;
}

// TODO: Start the PWM signal generation
void Servo::start_servo() {
	HAL_TIM_PWM_Start(this->m_tim_handle, this->m_channel);
}

// TODO: Move the servo to the specified angle
void Servo::set_servo_angle(int angle) {
	uint32_t ticks = (((angle + 90) /180) * 1000) + 999;
	__HAL_TIM_SET_COMPARE(this->m_tim_handle,this->m_channel, ticks);
}
