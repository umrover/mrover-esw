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
        lpuart = UART{LPUART, get_uart_options()};

        Logger::init(&lpuart);
        auto const& logger = Logger::instance();
        logger.info("Entering event loop...");

        while (true) {
            logger.info("looping...");
            HAL_Delay(1000);
        }

        event_loop();    
    }
}

extern "C" {
    void HAL_PostInit() {
        mrover::init();
    }
}