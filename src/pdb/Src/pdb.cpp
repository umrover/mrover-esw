#include <optional>
#include <MRoverCAN.hpp>

#include "pdb.hpp"
#include "pdb_config.hpp"
#include "main.h"

extern TIM_HandleTypeDef htim2;
extern FDCAN_HandleTypeDef hfdcan1;

namespace mrover {

    static constexpr TIM_HandleTypeDef* BLINK_TIM = &htim2; // 200ms
    static constexpr FDCAN_HandleTypeDef* HFDCAN = &hfdcan1;

    // Globals
    pdb_config_t config;
    bool volatile initialized = false;

    // Peripherals
    std::optional<PDB> pdb;
    std::optional<FDCAN> fdcan;

    void event_loop() {
        while (true) {}
    }

    void init() {
        fdcan.emplace(HFDCAN, get_can_options(&config));

        auto can_handler = MRoverCANHandler{&*fdcan};
        auto can_tx = Pin{CAN_TX_LED_GPIO_Port, CAN_TX_LED_Pin};
        auto can_rx = Pin{CAN_RX_LED_GPIO_Port, CAN_RX_LED_Pin};

        auto auton_led = AutonLED{
                Pin(AUTON_LED_R_GPIO_Port, AUTON_LED_R_Pin),
                Pin(AUTON_LED_G_GPIO_Port, AUTON_LED_G_Pin),
                Pin(AUTON_LED_B_GPIO_Port, AUTON_LED_B_Pin)};

        pdb.emplace(auton_led, BLINK_TIM, can_tx, can_rx, can_handler);

        initialized = true;
    }
} // namespace mrover

extern "C" {
void PostInit() {
    mrover::init();
    mrover::event_loop();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    if (htim == mrover::BLINK_TIM) {
        mrover::pdb->blink();
    }
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs) {
    // TODO(nate): move callback body to cpp
    if (!mrover::initialized)
        return;

    while (mrover::fdcan->messages_to_process() > 0) {
        mrover::pdb->handle_request();
    }
}
}
