#include "main.h"
#include "CO2Sensor.hpp"
#include <logger.hpp>
#include <config.hpp>

extern UART_HandleTypeDef hlpuart1;
extern I2C_HandleTypeDef hi2c3;

namespace mrover {
    static constexpr UART_HandleTypeDef* LPUART = &hlpuart1;
    I2C_HandleTypeDef* i2c = &hi2c3;

    UART lpuart;
    CO2Sensor co2_sensor;
    double co2 = 0;

    void init() {
        lpuart = UART{LPUART, get_uart_options()};

        Logger::init(&lpuart);
        auto const& logger = Logger::instance();

        co2_sensor = CO2Sensor(i2c);
        co2_sensor.init();

        logger.info("Entering event loop...");

        while (true) {
            HAL_Delay(2000);
            co2_sensor.request_co2();
            logger.info("CO2 ppm: %f", co2);
        }
    }
}

extern "C" {
    void HAL_PostInit() {
        mrover::init();
    }

    void HAL_I2C_MasterTxCpltCallback (I2C_HandleTypeDef* hi2c) {
        mrover::co2_sensor.receive_buf();
    }

    void HAL_I2C_MasterRxCpltCallback (I2C_HandleTypeDef* hi2c) {
        mrover::co2_sensor.update_co2();
        mrover::co2 = mrover::co2_sensor.get_co2();
    }
}