#include "CO2Sensor.hpp"
#include "OxygenSensor.hpp"
#include "OzoneSensor.hpp"
#include "stm32g4xx_hal_tim.h"
#include "thp_sensor.hpp"
#include <logger.hpp>
#include <config.hpp>

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef hlpuart1;
extern I2C_HandleTypeDef hi2c3;

namespace mrover {
    enum Sensor {
        sensor_co2 = 0,
        sensor_thp = 1,
        sensor_ozone = 2,
        sensor_oxygen = 3,
    };

    static constexpr TIM_HandleTypeDef* SENS_TIM = &htim2;
    static constexpr TIM_HandleTypeDef* CO2_TIM = &htim3;
    static constexpr UART_HandleTypeDef* LPUART = &hlpuart1;
    static constexpr I2C_HandleTypeDef* I2C = &hi2c3;

    UART lpuart;
    THP thp_sensor;
    CO2Sensor co2_sensor;
    THP_data thp_data;
    OzoneSensor ozone_sensor;
    OxygenSensor oxygen_sensor;
    Sensor current_sensor = sensor_co2;
    float co2 = 0;
    float ozone = 0;
    float oxygen = 0;

    void init() {
        lpuart = UART{LPUART, get_uart_options()};
        Logger::init(&lpuart);
        auto const& logger = Logger::instance();

        thp_sensor = THP{I2C};
	    thp_sensor.init();
    
        co2_sensor = CO2Sensor{I2C};
        co2_sensor.init();

        ozone_sensor = OzoneSensor(I2C);
	    ozone_sensor.init();

        oxygen_sensor = OxygenSensor(I2C);
	    oxygen_sensor.init();

        logger.info("Polling sensors...");

        HAL_TIM_Base_Start_IT(mrover::SENS_TIM);

        while (true) {}
    }

    void log_data() {
        static auto const& logger = Logger::instance();

        logger.info("co2: %f", mrover::co2);
        logger.info("temp: %f", mrover::thp_data.temp);
        logger.info("humidity: %f", mrover::thp_data.humidity);
        logger.info("pressure: %f", mrover::thp_data.pressure);
        logger.info("ozone: %f", mrover::ozone);
        logger.info("oxygen: %f", mrover::oxygen);
    }
}

extern "C" {
    void HAL_PostInit() {
        mrover::init();
    }

    void HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim) {
        if (htim == mrover::SENS_TIM) {
            // start requests for sensor data
            if (mrover::current_sensor == mrover::sensor_co2) {
                mrover::co2_sensor.request_co2();
            } else if (mrover::current_sensor == mrover::sensor_thp) {
                mrover::thp_sensor.read_thp();
            } else if (mrover::current_sensor == mrover::sensor_ozone) {
                mrover::ozone_sensor.receive_buf();
            } else if (mrover::current_sensor == mrover::sensor_oxygen) {
                mrover::oxygen_sensor.read_oxygen();
            }

            mrover::log_data();
        } else if (htim == mrover::CO2_TIM) {
            // handle thp sensor
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
        if (mrover::current_sensor == mrover::sensor_co2) {
            mrover::co2 = mrover::co2_sensor.update_co2();
            mrover::current_sensor = mrover::sensor_thp;
        }
    }

    void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
        if (mrover::current_sensor == mrover::sensor_thp) {
		    mrover::thp_data = mrover::thp_sensor.update_thp();
            mrover::current_sensor = mrover::sensor_ozone;
        } else if (mrover::current_sensor == mrover::sensor_ozone) {
            mrover::ozone = mrover::ozone_sensor.update_ozone();
            mrover::current_sensor = mrover::sensor_oxygen;
        } else if (mrover::current_sensor == mrover::sensor_oxygen) {
            mrover::oxygen = mrover::oxygen_sensor.update_oxygen();
            mrover::current_sensor = mrover::sensor_co2;
        }
	}
}