#pragma once

#include <optional>
#include <hw/analog_pin.hpp>

#include "main.h"

// Brief Code Overview:
// ---------------------------------------------------------------------------
// The sensor measures the voltage drop accross the shunt resistor (0.5mOhms) 
// and determines the current using Ohms law. The voltage drop is amplified by
// the sensor by a factor of 20 (by default), which is divided out in the     
// current calculation. `initialize` must me called to set the valid bool to  
// true, allowing data to be read. `update_sensor` will update the current    
// variable, and `current` will return the variable. Consider making these 
// into one function if prefered. 


namespace mrover {
    class AD8418A {
        AnalogPin m_analog_pin;
        bool m_valid{false};
        bool m_enabled{true};
        float m_gain{20.0f};
        float m_shunt_resistance{0.0005};
        float m_vref{3.3f};
        float m_vcm{1.65f};
        uint16_t m_adc_resolution{4095};
        float m_current{};
    public:
        AD8418A() = default;

        //-GMS - Create an instance with the sensor Analog Pin  
        explicit AD8418A(AnalogPin const& analogPin) : m_analog_pin{analogPin} {}

        /**
        * @brief Initializes the analog current sensor (AD8418A).
        *
        * Configures the sensor with the appropriate electrical parameters
        * needed to accurately convert the analog output voltage to current.
        * This must be called once before using `update_sensor()` to read current values.
        *
        * @param shunt_resistance  The resistance of the external shunt resistor, in ohms.
        *                          For example, use 0.001f for a 1 mΩ shunt.
        *
        * @param gain              The voltage gain of the amplifier. Default is 20.0f (20 V/V)
        *                          for the AD8418A.
        *
        * @param enabled           Whether the sensor should start enabled after initialization.
        *                          If false, the sensor will remain inactive until `enable()` is called.
        *
        * @param voltage_reference The ADC reference voltage in volts (typically 3.3f or 5.0f),
        *                          used to convert raw ADC readings to actual voltage values.
        *
        * @param adc_resolution    The maximum numeric value of the ADC conversion (e.g. 4095 for 12-bit ADC,
        *                          1023 for 10-bit ADC).
        */
        auto initialize(float shunt_resistance=0.0005, float gain = 20.0f,
                        bool enabled = true, float voltage_reference = 3.3f,
                        uint16_t adc_resolution = 4095) -> void {
            m_valid = true;
            m_enabled = enabled;
            m_shunt_resistance = shunt_resistance;
            m_gain = gain;
            m_vref = voltage_reference;
            m_vcm = voltage_reference / 2;
            m_adc_resolution = adc_resolution;
            m_current = 0.0f;
        }



        /**
        * @brief Updates the current measurement from the AD8418A sensor.
        *
        * Reads the analog voltage output from the AD8418A amplifier using the configured
        * ADC channel, then converts this voltage into a current value based on the
        * amplifier gain and shunt resistor value. The computed current is stored
        * internally and can be accessed via `current()`.
        *
        * @note This function should be called periodically (e.g., inside a control or
        *       telemetry loop) to refresh the current reading. If the sensor is disabled
        *       or invalid, this function will return immediately without updating.
        */
        auto update_sensor() -> void {
            if (!m_enabled || !m_valid) return;
            auto adc_value = m_analog_pin.read_analog();
            float v_out = (static_cast<float>(adc_value) / static_cast<float>(m_adc_resolution)) * m_vref;
            float v_sense = v_out - m_vcm; // delete later -- doesn't seem to have a purpose
            m_current = v_out / (m_gain * m_shunt_resistance);
        }

        /**
        * @brief Returns the most recently measured current value.
        *
        * Requires `update_sensor()` to be called before to get an 
        * accurate reading.
        * 
        * @return float The measured current in amperes (A).
        *
        * @note Only updated when `update_sensor()` is called. If 
        *       `update_sensor()` has not been run, this will return
        *        the previous (or zero) reading.
        */
        [[nodiscard]] auto current() const -> float { return m_current; }

        /**
        * @brief Indicates whether the current sensor has been properly initialized.
        *
        * Used to confirm that the sensor has been configured via `initialize()`.
        * Set to true once initialized and remains true unless the sensor object
        * is reset or reinitialized incorrectly.
        *
        * @return bool True if the sensor has been initialized and is ready for use,
        *              false otherwise.
        */
        [[nodiscard]] auto valid() const -> bool { return m_valid; }

        /**
        * @brief Sets enabled bool to true, allowing sensor readings
        *
        * Used to toggle the value of the enabled bool, ensuring the sensor
        * is enabled, allowing measurements can be taken from it.
        */
        auto enable() -> void { m_enabled = true; }
        
        /**
        * @brief Sets enabled bool to false, disabling sensor readings
        *
        * Used to toggle the value of the enabled bool, ensuring the sensor
        * is disabled, blocking measurements can be taken from it. Also resets
        * last measured current to 0 to reset the state of the sensor.
        */
        auto disable() -> void { m_enabled = false; m_current = 0.0f; }
    };
} // namespace mrover
