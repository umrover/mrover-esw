#include "ScienceSensor.hpp"
#include <adc.hpp>
#include <cstdint>

namespace mrover {
    class UVSensor : public ScienceSensor {
    private:
        ADCBase* m_adc;
        uint8_t m_channel;
        float m_uv_index{0.0};
        uint16_t m_adc_res{4095};

    public:
        // default constructor
        UVSensor() = default;

        // adc_in is a pointer to the desired adc and channel_in is the channel that the UVsensor is on
        UVSensor(ADCBase* adc_in, uint8_t const channel_in)
            : m_adc(adc_in), m_channel(channel_in) {}

        // returns the current value of uv_index
        [[nodiscard]]  float get_uv() const {
            return m_uv_index;
        }

        // updates the value of the sensor
        void update() override {
            m_uv_index = 33.0 * ((float)m_adc->get_channel_value(m_channel) / (float)m_adc_res);
        }

        // polls the sensor for data
        void poll() override {
            m_adc->start();
        }

        // attempts to initialize sensor, returns true on success and false on failure
        bool init() override {
            return true;
        }
    }; // class UVSensor
} // namespace mrover