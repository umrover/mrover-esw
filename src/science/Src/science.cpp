#include "main.h"
#include <logger.hpp>
#include <config.hpp>

extern UART_HandleTypeDef hlpuart1;

namespace mrover {
    static constexpr UART_HandleTypeDef* LPUART = &hlpuart1;

    UART lpuart;

    void event_loop() {
        while (true) {}
    }

    void init() {
        // set debug led 1 high
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

        lpuart = UART{LPUART, get_uart_options()};

        Logger::init(&lpuart);
        auto const& logger = Logger::instance();
        logger.info("Entering event loop...");

        while (true) {
            logger.info("looping...");
        }

        event_loop();    
    }
}

extern "C" {
    void HAL_PostInit() {
        mrover::init();
    }

    // void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {

    // }

    // void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
        
    // }
}