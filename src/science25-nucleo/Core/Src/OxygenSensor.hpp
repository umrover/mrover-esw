#include "main.h"
#include "hardware_adc.hpp"

namespace mrover {
	class OxygenSensor {
	private:
		uint8_t dev_addr;
		uint8_t mem_addr;
		uint8_t* buf;
		uint16_t size;
		I2C_HandleTypeDef *i2c;
		float ppm;

	public:
		OxygenSensor (uint8_t dev_in, uint8_t mem_in, uint8_t* buf_in, uint16_t size_in, I2C_HandleTypeDef *i2c_in)
			: dev_addr(dev_in), mem_addr(mem_in), buf(buf_in), size(size_in), i2c(i2c_in), ppm(0) {
		};

		float update_oxygen() {
			HAL_StatusTypeDef status;
			status = HAL_I2C_Mem_Read(i2c, dev_addr << 1, mem_addr, 1, buf, size, 100);

			if (status != HAL_OK)
			{
				uint32_t x = HAL_I2C_GetError(&hi2c2);
				HAL_I2C_DeInit(i2c);
				HAL_Delay(5);
				HAL_I2C_Init(i2c);
			}

			ppm = ((20.9 / 120.0) * (((float)buf[0]) + ((float)buf[1] / 10.0) + ((float)buf[2] / 100.0))); // Could be that the 20.9/120.00 is not the correct value for _Key
			return ppm;
		}

		float get_oxygen() {
			return ppm;
		}
	}; // class UVSensor
} // namespace mrover
