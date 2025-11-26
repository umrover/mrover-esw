#include <cstdint>

#include <hw/hbridge.hpp>
#include <logger.hpp>

#include "main.h"
#include "motor.hpp"
#include "stm32g431xx.h"
#include "stm32g4xx_hal_tim.h"

#include <functional>

extern UART_HandleTypeDef hlpuart1;
extern FDCAN_HandleTypeDef hfdcan1;

extern I2C_HandleTypeDef hi2c1;

/**
 * For each repeating timer, the update rate is determined by the .ioc file.
 *
 * Specifically the ARR value. You can use the following equation: ARR = (MCU Clock Speed) / (Update Rate) / (Prescaler + 1) - 1
 * For the STM32G4 we have a 140 MHz clock speed configured.
 *
 * You must also set auto reload to true so the interrupt gets called on a cycle.
 */

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
// extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim8;
extern TIM_HandleTypeDef htim15;
extern TIM_HandleTypeDef htim16;
extern TIM_HandleTypeDef htim17;


namespace mrover {

    Pin can_tx;
    Pin can_rx;

    Motor motor;

    auto init() -> void {
        Logger::init(&hlpuart1);

        can_tx = Pin{CAN_TX_LED_GPIO_Port, CAN_TX_LED_Pin};
        can_rx = Pin{CAN_RX_LED_GPIO_Port, CAN_RX_LED_Pin};

        motor = Motor{
            HBridge{&htim1, TIM_CHANNEL_1, Pin{MOTOR_DIR_GPIO_Port, MOTOR_DIR_Pin}},
        };

        Logger::get_instance()->info("initialized");
    }

    [[noreturn]] auto loop() -> void {

        size_t n = 0;
        for ( ;; ) {
            Logger::get_instance()->info("hi, i'm a BMC :) %u", n++);
            motor.write(15_percent);
            HAL_Delay(2500);
            motor.write(-15_percent);
            HAL_Delay(2500);
            if (n == 3) break;
        }

        motor.write(0_percent);

        for ( ;; ) {
            Logger::get_instance()->info("hi, i'm a BMC :) %u", n++);
            can_tx.set();
            can_rx.reset();
            HAL_Delay(2500);
            can_tx.reset();
            can_rx.set();
            HAL_Delay(2500);
        }
    }

} // namespace mrover

extern "C" {

void PostInit() {
    mrover::init();
}

void Loop() {
    mrover::loop();
}

/**
 * These are interrupt handlers. They are called by the HAL.
 *
 * These are set up in the .ioc file.
 * They have to be enabled in the NVIC settings.
 * It is important th
*/

/**
 * \note Timers have to be started with "HAL_TIM_Base_Start_IT" for this interrupt to work for them.
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {

}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef* htim) {

}

/**
 * \note FDCAN1 has to be configured with "HAL_FDCAN_ActivateNotification" and started with "HAL_FDCAN_Start" for this interrupt to work.
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs) {

}

// TODO(eric): Error handling

void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef* hfdcan) {}

void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef* hfdcan, uint32_t ErrorStatusITs) {}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef* hi2c) {
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* hi2c) {
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c) {}
}
