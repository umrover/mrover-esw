#include "main.h"
#include <limits>

#define TEMP_HUM_ADDRESS_1 0x76
#define TEMP_HUM_ADDRESS_2 0x77

namespace mrover {
	class TempHumPressureSensor{
	private:
		double temp;
		double humidity;
		double pressure;
        uint8_t dev_addr;
        I2C_HandleTypeDef* i2c;

		void calculate_temp(){

		}

		void calculate_hum(){

		}

		void calculate_pressure(){

		}

	public:
		TempHumPressureSensor() = default;

		TempHumPressureSensor(I2C_HandleTypeDef* i2c_in)
		            : temp(0), humidity(0), dev_addr(TEMP_HUM_ADDRESS_1), i2c(i2c_in) {
		              };

		void set_settings(){

		}

		void set_mode(){

		}

		void get_compensation_val(){

		}



        double get_current_temp() {
            return temp;
        }

        double get_current_humidity() {
            return humidity;
        }

        double get_current_pressure() {
			return pressure;
		}
	}; // class TempHumiditySensor
} // namespace mrover
