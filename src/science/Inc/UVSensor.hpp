#include "ScienceSensor.hpp"
#include <adc.hpp>
#include <cstdint>


    #define UVINDEX_SENSOR_ADDR 0x23
    #define IVS_DATA 0x06
    #define UVS_INDEX 0x07



namespace mrover {
    class UVSensor : public ScienceSensor {
    private:
        SMBus* m_smbus; //I2C handle pointer
        uint8_t m_rx_buff[2]; // I2C recieve buffer
        uint16_t UV_Index_data;


    public:
        // default constructor
        UVSensor() = default;

        UVSensor(SMBus* smbus_in) 
            : m_smbus(smbus_in) {}


        // returns the current value of uv_index
        [[nodiscard]]  float get_uv() const {
            return UV_Index_data;
        }

        // updates the value of the sensor
        void update() override {
            m_smbus->asynch_mem_read(UVINDEX_SENSOR_ADDR, UVS_INDEX, I2C_MEMADD_SIZE_8BIT, m_rx_buff);
        
        }

        // polls the sensor for data
        void poll() override {
            UV_Index_data = (uint16_t)(m_rx_buff[0]<<8) | m_rx_buff[1];
        }

        // attempts to initialize sensor, returns true on success and false on failure
        bool init() override {
            return true;
        }
    }; // class UVSensor
} // namespace mrover