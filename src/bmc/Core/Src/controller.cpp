#include <cstdint>
#include <functional>

#include <hw/ad8418a.hpp>
#include <hw/hbridge.hpp>
#include <logger.hpp>
#include <serial/fdcan.hpp>
#include <timer.hpp>

#include "config.hpp"
#include "main.h"
#include "motor.hpp"
#include "stm32g431xx.h"
#include "stm32g4xx_hal_tim.h"

#include "CANBus1.hpp"


extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef hlpuart1;
extern FDCAN_HandleTypeDef hfdcan1;

extern TIM_HandleTypeDef htim1;
// extern TIM_HandleTypeDef htim2;
// extern TIM_HandleTypeDef htim3;
// extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim6;
// extern TIM_HandleTypeDef htim8;
// extern TIM_HandleTypeDef htim15;
extern TIM_HandleTypeDef htim16;
extern TIM_HandleTypeDef htim17;


namespace mrover {

    static constexpr uint8_t NUM_ADC_CHANNELS = 1;

    static constexpr UART_HandleTypeDef* LPUART_1 = &hlpuart1;
    static constexpr ADC_HandleTypeDef* ADC_1 = &hadc1;
    static constexpr FDCAN_HandleTypeDef* FDCAN_1 = &hfdcan1;

    static constexpr TIM_HandleTypeDef* MOTOR_PWM_TIM = &htim1;
    static constexpr TIM_HandleTypeDef* TX_TIM = &htim6;        // 10 Hz
    static constexpr TIM_HandleTypeDef* CAN_WWDG_TIM = &htim16; // 0.1 Hz
    static constexpr TIM_HandleTypeDef* CONTROL_TIM = &htim17;  // 100 Hz

    bmc_config_t config;
    bool initialized = false;

    // Peripherals
    UART lpuart;
    ADC<NUM_ADC_CHANNELS> adc;
    FDCAN fdcan;

    // Timers
    Timer tx_tim;
    Timer can_wwdg_tim;
    Timer control_tim;

    // Hardware Units
    Pin can_tx;
    Pin can_rx;
    CANBus1Handler can_receiver;
    Motor motor;

    // Global Configs
    // TODO(eric): really nasty that this has to be global
    FDCAN::Filter can_node_filter;

    /**
     * Send a CAN message defined in CANBus1.dbc on the bus.
     * @param msg CAN message to send
     */
    auto send_can_message(CANBus1Msg_t const& msg) -> void {
        if (!initialized) return;
        can_tx.set();
        can_receiver.send(msg, config.get<bmc_config_t::can_id>(), config.get<bmc_config_t::host_can_id>());
        Logger::instance().debug("CAN Message Sent");
        can_tx.reset();
    }

    /**
     * Receive and parse a CAN message over the bus.
     * Message should be of a type defined in CANBus1.dbc
     */
    auto receive_can_message() -> void {
        if (!initialized) return;

        // start the CAN watchdog if it lapsed
        if (!can_wwdg_tim.is_enabled()) {
            can_wwdg_tim.start();
        }

        while (fdcan.messages_to_process() > 0) {
            if (auto const recv = can_receiver.receive(); recv) {
                can_rx.set();
                auto const& msg = *recv;
                motor.receive(msg);
                can_wwdg_tim.reset();
                can_rx.reset();
            }
        }
    }

    /**
     * Initialization sequence for BMC.
     */
    auto init() -> void {
        // initialize peripherals
        lpuart = UART{LPUART_1, get_uart_options()};
        adc = ADC<NUM_ADC_CHANNELS>{ADC_1, get_adc_options()};
        fdcan = FDCAN{FDCAN_1, get_can_options(&config, &can_node_filter)};

        // initialize logger
        Logger::init(&lpuart);
        auto const& logger = Logger::instance();
        logger.info("Initializing");

        // setup debug LEDs
        logger.info("...CAN LEDs");
        can_tx = Pin{CAN_TX_LED_GPIO_Port, CAN_TX_LED_Pin};
        can_rx = Pin{CAN_RX_LED_GPIO_Port, CAN_RX_LED_Pin};

        // setup can transceiver
        logger.info("...CAN Transceiver");
        can_receiver = CANBus1Handler{&fdcan};

        // setup motor instance
        logger.info("...Motor");
        motor = Motor{
                HBridge{MOTOR_PWM_TIM, TIM_CHANNEL_1, Pin{MOTOR_DIR_GPIO_Port, MOTOR_DIR_Pin}},
                AD8418A{&adc, ADC_CHANNEL_0},
                send_can_message,
                &config,
        };

        // setup timers
        logger.info("...Timers");
        tx_tim = Timer{TX_TIM, true, "TX Timer"};                       // transmit timer (on interrupt)
        can_wwdg_tim = Timer{CAN_WWDG_TIM, true, "CAN Watchdog Timer"}; // can watchdog timer (on interrupt)
        control_tim = Timer{CONTROL_TIM, true, "Control Timer"};        // control timer (update driven output, on interrupt)

        // set initialization state and initial error state
        logger.info("BMC Initialized");
        initialized = true;
    }

    /**
     * Callback for timer periods elapsing.
     * Timers have to be started with "HAL_TIM_Base_Start_IT" for this interrupt to work for them.
     * @param htim The timer whose period elapsed
     */
    auto timer_elapsed_callback(TIM_HandleTypeDef const* htim) -> void {
        if (!initialized) return;
        if (htim == TX_TIM) {
            motor.send_state();
        } else if (htim == CAN_WWDG_TIM) {
            can_wwdg_tim.stop();
            motor.tx_watchdog_lapsed();
            Logger::instance().warn("TX Watchdog Lapsed");
        } else if (htim == CONTROL_TIM) {
            motor.drive_output();
        }
    }

    /**
     * Callback enabling asynchronous serial logs via UART/DMA.
     * @param huart UART handle from callback
     */
    auto uart_tx_callback(UART_HandleTypeDef const* huart) -> void {
        if (huart == LPUART_1) {
            lpuart.handle_tx_complete();
        }
    }

} // namespace mrover

extern "C" {

void PostInit() {
    mrover::init();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    mrover::timer_elapsed_callback(htim);
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs) {
    mrover::receive_can_message();
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
    mrover::uart_tx_callback(huart);
}

// TODO(eric) implement
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef* htim) {}
void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef* hfdcan) {}
void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef* hfdcan, uint32_t ErrorStatusITs) {}
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef* hi2c) {}
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* hi2c) {}
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c) {}
}
