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
	private:
		I2C_HandleTypeDef* i2c;
		double ozone;
		int ozone_buf[OCOUNT];
		int collect_num = 20;
		int num_samples_collected = 0;
		uint8_t dev_addr;
		uint8_t M_Flag = 0;

		uint8_t rx_buf[10];

		void i2cWrite(uint8_t reg, uint8_t pData) {
			HAL_StatusTypeDef status;
			uint8_t cmd[2] = {reg, pData};
			status = HAL_I2C_Master_Transmit_IT(i2c, (dev_addr << 1), cmd, 2);
            if (status != HAL_OK) {
            	HAL_I2C_DeInit(i2c);
				HAL_I2C_Init(i2c);
                ozone = std::numeric_limits<double>::quiet_NaN();
                return;
            }
		}

		double get_average_num(int bArray[], int iFilterLen) {
			unsigned long bTemp = 0;
			for (int i = 0; i < iFilterLen; i++) {
				bTemp += bArray[i];
			}
			return bTemp / iFilterLen;
		}

	public:
		OzoneSensor() = default;

		OzoneSensor(I2C_HandleTypeDef* i2c_in) : ozone(0.0), dev_addr(OZONE_ADDRESS_0), i2c(i2c_in) {}

		void set_collect_num(int n) {
			if (n <= 0 || n > OCOUNT) {
				collect_num = std::numeric_limits<double>::quiet_NaN();
			}
			collect_num = n;
		}

		void update_ozone() {
			for (uint8_t j = collect_num - 1; j > 0; j--) {
				ozone_buf[j] = ozone_buf[j-1];
			}

			switch (M_Flag) {
			case 0:
				i2cWrite(SET_PASSIVE_REGISTER, AUTO_READ_DATA);
				break;
			case 1:
				i2cWrite(SET_PASSIVE_REGISTER, PASSIVE_READ_DATA);
				break;
			}

			if (num_samples_collected < collect_num) {
				num_samples_collected++;
			}
		}

		void receive_buf() {
			HAL_StatusTypeDef status;
			uint8_t reg;
			switch (M_Flag) {
			case 0:
				reg = AUTO_DATA_HIGE_REGISTER;
				break;
			case 1:
				reg = PASS_DATA_HIGE_REGISTER;
				break;
			}

			status = HAL_I2C_Master_Transmit_IT(i2c, (dev_addr << 1), &reg, 1);
			if (status != HAL_OK) {
				HAL_I2C_DeInit(i2c);
				HAL_I2C_Init(i2c);
				ozone = std::numeric_limits<double>::quiet_NaN();
				return;
			}

			HAL_Delay(100);

			status = HAL_I2C_Master_Receive_IT(i2c, (dev_addr << 1) | 0x01, rx_buf, 10);
			if (status != HAL_OK) {
				HAL_I2C_DeInit(i2c);
				HAL_I2C_Init(i2c);
				ozone = std::numeric_limits<double>::quiet_NaN();
				return;
			}
		}

		void setModes(uint8_t mode) {
			if(mode == MEASURE_MODE_AUTOMATIC){
				i2cWrite(MODE_REGISTER , MEASURE_MODE_AUTOMATIC);
				M_Flag = 0;
			} else if(mode == MEASURE_MODE_PASSIVE){
				i2cWrite(MODE_REGISTER , MEASURE_MODE_PASSIVE);
				M_Flag = 1;
			} else {
				return;
			}
		}

		void calculate_ozone() {
			ozone_buf[0] = ((int16_t)rx_buf[0] << 8) + rx_buf[1];
			ozone = get_average_num(ozone_buf, num_samples_collected);
		}

		double get_ozone() {
			return ozone;
		}
	};

}
