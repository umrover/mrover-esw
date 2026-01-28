#include "main.h"
#include "CO2Sensor.hpp"
#include "stm32g4xx_hal.h"
#include "thp_sensor.hpp"
#include <logger.hpp>
#include <config.hpp>

extern UART_HandleTypeDef hlpuart1;
extern I2C_HandleTypeDef hi2c3;

namespace mrover {
    static constexpr UART_HandleTypeDef* LPUART = &hlpuart1;
    static constexpr I2C_HandleTypeDef* I2C = &hi2c3;

    UART lpuart;
    // THP thp_sensor;
    CO2Sensor co2_sensor;
    // THP_data thp_data;
    double co2 = 0;

    void init() {
        lpuart = UART{LPUART, get_uart_options()};

        Logger::init(&lpuart);
        auto const& logger = Logger::instance();

        // thp_sensor = THP(&hi2c3);
	    // thp_sensor.init();
    
        co2_sensor = CO2Sensor(I2C);
        co2_sensor.init();

        logger.info("Polling sensor...");

        co2_sensor.request_co2();

        while (true) {
            // HAL_Delay(1000);
            // thp_sensor.read_thp();
            // logger.info("temp: %f", thp_data.temp);
            // logger.info("humidity: %f", thp_data.humidity);
            // logger.info("pressure: %f", thp_data.pressure);

            logger.info("co2: %f", co2);
        }
    }
}

extern "C" {
    void HAL_PostInit() {
        mrover::init();
    }

    void HAL_I2C_MasterTxCpltCallback (I2C_HandleTypeDef* hi2c) {
        for (int i = 0; i < 1000000; i++);
        mrover::co2_sensor.receive_buf();
    }

    void HAL_I2C_MasterRxCpltCallback (I2C_HandleTypeDef* hi2c) {
        mrover::co2_sensor.update_co2();
        mrover::co2 = mrover::co2_sensor.get_co2();
        mrover::co2_sensor.request_co2();
    }

    // void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
	// 	mrover::thp_sensor.update_thp();
    //     mrover::thp_data = mrover::thp_sensor.get_thp();
	// }
}