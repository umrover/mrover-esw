#include "ScienceSensor.hpp"
#include <adc.hpp>
#include <cstdint>

namespace mrover {
    class UVSensor : public ScienceSensor {
    private:
        ADCBase* adc;
        uint8_t channel;
        float uv_index{0.0};
        uint16_t adc_res{4095};

    public:
        // default constructor
        UVSensor() = default;

        // adc_in is a pointer to the desired adc and channel_in is the channel that the UVsensor is on
        UVSensor(ADCBase* adc_in, uint8_t const channel_in)
            : adc(adc_in), channel(channel_in) {}

        // returns the current value of uv_index
        [[nodiscard]]  float get_uv() const {
            return uv_index;
        }

        // updates the value of the sensor
        void update() override {
            uv_index = 33.0 * ((float)adc->get_channel_value(channel) / (float)adc_res);
        }

        // polls the sensor for data
        void poll() override {
            adc->start();
        }

        // attempts to initialize sensor, returns true on success and false on failure
        bool init() override {
            return true;
        }
    }; // class UVSensor
} // namespace mrover