#include "main.h"
#include "stdio.h"

#define DATA_REGS 0x32 // Data registers starting address, x = 0x32, y = 0x34, z = 0x36
#define PWR_CTL_REG 0x2D // Power control register, needed to start measurements

namespace mrover {
	// struct representing an accelerometers data for the x, y, and z axis
	struct AccelData {
		float x;
		float y;
		float z;
	};

	// enum for axis
	enum class AxisEnum {
		X_AXIS = 0,
		Y_AXIS = 1,
		Z_AXIS = 2,
	};

	uint8_t current_sensor;

    class ADXL343 {
    private:
        I2C_HandleTypeDef* i2c; // pointer to i2c handle
        uint8_t dev_addr; // i2c address of the accelerometer, either 0x1D (SDO high) or 0x53 (SDO low)
        uint8_t accel_id; // id of the accelerometer 1 to number of accelerometers
        AccelData accel_data; // accelerometer data
        uint8_t accel_buffer[6]; // buffer for reading from sensor

    public:
        ADXL343() = default;

        // initialize accelerometer
        ADXL343(I2C_HandleTypeDef* i2c_in, uint8_t addr_in, uint8_t id_in)
            : i2c(i2c_in), dev_addr(addr_in), accel_id(id_in), accel_data({0, 0, 0}) {
              };

        // read data from data register on accelerometer asynchronously
        void read_data() {
        	current_sensor = accel_id;
            HAL_StatusTypeDef status;
            status = HAL_I2C_Mem_Read_IT(i2c, (dev_addr << 1), DATA_REGS, I2C_MEMADD_SIZE_8BIT, accel_buffer, 6);
            if (status != HAL_OK) {
            	HAL_I2C_DeInit(i2c);
				HAL_I2C_Init(i2c);
                return;
            }
        }

        // update accel_data with data received from read, divide by 256 to get g's
        void update_accel() {
        	accel_data.x = (int16_t)(accel_buffer[1] << 8 | accel_buffer[0]) / 256.00;
        	accel_data.y = (int16_t)(accel_buffer[3] << 8 | accel_buffer[2]) / 256.00;
        	accel_data.z = (int16_t)(accel_buffer[5] << 8 | accel_buffer[4]) / 256.00;
        	//For proof of concept, not final code structure
        	//printf("x: \n\r");
        	//printf("%f \n\r", accel_data.x);
        	//printf("y: \n\r");
			//printf("%f \n\r", accel_data.y);
			//printf("z: \n\r");
			//printf("%f \n\r",  accel_data.z);
        }


        AccelData getData(){
        	return accel_data;
        }

        /*float getX(){
        	return accel_data.x;
        }

        float getY(){
			return accel_data.y;
		}

        float getZ(){
			return accel_data.z;
		}*/

        // starts measurements for an accelerometer
        HAL_StatusTypeDef start_accel() {
        	HAL_StatusTypeDef status;
        	uint8_t meas_cmd[1] = {0x08};
        	status = HAL_I2C_Mem_Write(i2c, (dev_addr << 1), PWR_CTL_REG, I2C_MEMADD_SIZE_8BIT, meas_cmd, 1, HAL_MAX_DELAY);
        	if (status != HAL_OK) {
				HAL_I2C_DeInit(i2c);
				HAL_I2C_Init(i2c);
			}
        	return status;
        }

        // returns the value of a given axis in accel_data
        float get_axis(AxisEnum axis) {
        	switch (axis) {
        	case AxisEnum::X_AXIS:
        		return accel_data.x;
        	case AxisEnum::Y_AXIS:
				return accel_data.y;
        	case AxisEnum::Z_AXIS:
				return accel_data.z;
        	}
        }

        // returns a const reference to accel_data
        const AccelData& get_accel_data() const {
        	return accel_data;
        }
    }; // class ADXL343
} // namespace mrover
