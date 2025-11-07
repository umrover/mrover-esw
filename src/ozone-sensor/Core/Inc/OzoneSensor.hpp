#include "main.h"
#include <limits>
#include <stdexcept>

#define           OZONE_ADDRESS_0           0x70
#define           OZONE_ADDRESS_1           0x71
#define           OZONE_ADDRESS_2           0x72
#define           OZONE_ADDRESS_3           0x73
#define           MEASURE_MODE_AUTOMATIC    0x00           ///< active  mode
#define           MEASURE_MODE_PASSIVE      0x01           ///< passive mode
#define           AUTO_READ_DATA            0x00           ///< auto read ozone data
#define           PASSIVE_READ_DATA         0x01           ///< passive read ozone data
#define           MODE_REGISTER             0x03           ///< mode register
#define           SET_PASSIVE_REGISTER      0x04           ///< read ozone data register
#define           AUTO_DATA_HIGE_REGISTER   0x09           ///< AUTO data high eight bits
#define           AUTO_DATA_LOW_REGISTER    0x0A           ///< AUTO data Low  eight bits
#define           PASS_DATA_HIGE_REGISTER   0x07           ///< AUTO data high eight bits
#define           PASS_DATA_LOW_REGISTER    0x08           ///< AUTO data Low  eight bits
#define           OCOUNT                    100            ///< Ozone Count Value

namespace mrover {

	class OzoneSensor {
	//Returns ozone in ppm
	private:
		I2C_HandleTypeDef* i2c;
		double ozone;
		int ozone_buf[OCOUNT];
		uint8_t dev_addr;

		uint8_t rx_buf[10];
		uint8_t send_data;

		void i2cWrite(uint8_t reg, uint8_t pData) {
			HAL_StatusTypeDef status;
			send_data = pData;
			status = HAL_I2C_Mem_Write_IT(i2c, (dev_addr << 1), reg, 1, &send_data, 1);
            if (status != HAL_OK) {
            	HAL_I2C_DeInit(i2c);
				HAL_I2C_Init(i2c);
                ozone = std::numeric_limits<double>::quiet_NaN();
                return;
            }
		}

	public:
		OzoneSensor() = default;
		bool read = false;

		OzoneSensor(I2C_HandleTypeDef* i2c_in) : ozone(0.0), dev_addr(OZONE_ADDRESS_3), i2c(i2c_in) {}



		void update_ozone1() {
			i2cWrite(SET_PASSIVE_REGISTER, PASSIVE_READ_DATA);
			read = true;
		}


		void receive_buf(){
			HAL_StatusTypeDef status;
			status = HAL_I2C_Mem_Read_IT(i2c, dev_addr << 1, PASS_DATA_HIGE_REGISTER, 1, rx_buf, 2);
			if (status != HAL_OK) {
				HAL_I2C_DeInit(i2c);
				HAL_I2C_Init(i2c);
				ozone = std::numeric_limits<double>::quiet_NaN();
				return;
			}
		}

		void setPassive(){
			i2cWrite(MODE_REGISTER , MEASURE_MODE_PASSIVE);

		}

		void calculate_ozone() {
			ozone_buf[0] = ((int16_t)rx_buf[0] << 8) + rx_buf[1];
			ozone = ozone_buf[0]/1000.0;
		}

		double get_ozone() {
			return ozone;
		}
	};

}
