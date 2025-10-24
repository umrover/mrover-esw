#include "servo.hpp"

// TODO: Implement the Servo constructor
Servo::Servo(TIM_HandleTypeDef* tim_handle, uint32_t channel) {
	m_tim_handle = tim_handle;
	m_channel = channel;
}

// TODO: Start the PWM signal generation
void Servo::start_servo() {
	HAL_TIM_PWM_Start(m_tim_handle, m_channel);
}

// TODO: Move the servo to the specified angle
void Servo::set_servo_angle(int angle) {
	if (angle == 90) {
		__HAL_TIM_SET_COMPARE(m_tim_handle, m_channel, 2000);
	} else if (angle == -90) {
		__HAL_TIM_SET_COMPARE(m_tim_handle, m_channel, 1000);
	} else {
		__HAL_TIM_SET_COMPARE(m_tim_handle, m_channel, 1500);
	}
}
