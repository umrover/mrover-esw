#include <cstdint>

#include "hbridge.hpp"

#include "main.h"

extern FDCAN_HandleTypeDef hfdcan1;

extern I2C_HandleTypeDef hi2c1;
#define ABSOLUTE_I2C &hi2c1

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

    HBridge hbridge{}; // TODO stuff goes here too probably
    // NOTE this is for EHW's hbridge circuit, might not be a bad idea to write a separate header for the LN298 hbridges laying around too

    auto init() -> void {
        // TODO test hbridge
    }

    auto loop() -> void {
        // TODO test hbridge
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
