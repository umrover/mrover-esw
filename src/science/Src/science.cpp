#include "CO2Sensor.hpp"
#include "stm32g4xx_hal_tim.h"
#include "thp_sensor.hpp"
#include <logger.hpp>
#include <config.hpp>

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef hlpuart1;
extern I2C_HandleTypeDef hi2c3;

namespace mrover {
    static constexpr TIM_HandleTypeDef* SENS_TIM = &htim2;
    static constexpr TIM_HandleTypeDef* CO2_TIM = &htim3;
    static constexpr UART_HandleTypeDef* LPUART = &hlpuart1;
    static constexpr I2C_HandleTypeDef* I2C = &hi2c3;

    UART lpuart;
    THP thp_sensor;
    CO2Sensor co2_sensor;
    THP_data thp_data;
    double co2 = 0;

    void init() {
        lpuart = UART{LPUART, get_uart_options()};

        Logger::init(&lpuart);
        auto const& logger = Logger::instance();

        thp_sensor = THP{I2C};
	    thp_sensor.init();
    
        co2_sensor = CO2Sensor{I2C};
        co2_sensor.init();

        logger.info("Polling sensors...");

        HAL_TIM_Base_Start_IT(mrover::SENS_TIM);

        while (true) {}
    }
}

extern "C" {
    void HAL_PostInit() {
        mrover::init();
    }

    void HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim) {
        static bool is_co2 = true;
        if (htim == mrover::SENS_TIM) {
            // start requests for sensor data
            if (is_co2)
                mrover::co2_sensor.request_co2();
            else
                mrover::thp_sensor.read_thp();

            is_co2 = !is_co2;
        } else if (htim == mrover::CO2_TIM) {
            // handle CO2 sensor
            mrover::co2_sensor.receive_buf();
            HAL_TIM_Base_Stop_IT(mrover::CO2_TIM);
        }
    }

    void HAL_I2C_MasterTxCpltCallback (I2C_HandleTypeDef* hi2c) {
        // set timer interrupt to read data when ready
        __HAL_TIM_SET_COUNTER(mrover::CO2_TIM, 0);
        HAL_TIM_Base_Start_IT(mrover::CO2_TIM);
    }

    void HAL_I2C_MasterRxCpltCallback (I2C_HandleTypeDef* hi2c) {
        // get updated co2
        mrover::co2 = mrover::co2_sensor.update_co2();

        auto const& logger = mrover::Logger::instance();
        logger.info("co2: %f", mrover::co2);
    }

    void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
        // get updated thp
		mrover::thp_data = mrover::thp_sensor.update_thp();

        auto const& logger = mrover::Logger::instance();
        logger.info("temp: %f", mrover::thp_data.temp);
        logger.info("humidity: %f", mrover::thp_data.humidity);
        logger.info("pressure: %f", mrover::thp_data.pressure);
	}
}