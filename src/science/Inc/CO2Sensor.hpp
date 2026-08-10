#include "ScienceSensor.hpp"
#include <serial/smbus.hpp>
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_def.h"

#define CO2_ADDR 0x29

namespace mrover {
	enum Mode {
		TX = 0,
		RX = 1,
	};

	class CO2Sensor : public ScienceSensor{
	private:
		SMBus* m_smbus;
		uint8_t m_req_buf[2];
        uint8_t m_rx_buf[2];
		float m_percent; // ozone value in ppm
		Mode m_mode;

		// requests raw co2 data over i2c, when sensor responds with data callback will be hit and will call receive_buf
		void request_co2() {
			m_smbus->async_transmit(CO2_ADDR, {reinterpret_cast<const char*>(m_req_buf), sizeof(m_req_buf)});
		}

		// receives raw ozone data over i2c
		void receive_buf() {
			m_smbus->async_receive(CO2_ADDR, m_rx_buf);
		}

	public:
		CO2Sensor() = default;

		explicit CO2Sensor (SMBus* smbus_in)
			: m_smbus(smbus_in), m_req_buf{0x36, 0x39}, m_rx_buf{0x00, 0x00}, m_percent(0.0), m_mode(Mode::TX) {}

		// returns the current ozone data in ppm
		[[nodiscard]] float get_co2() const {
			return m_percent;
		}

		// updates the value of the sensor
        void update() override {
			uint16_t raw = (m_rx_buf[0] << 8) | m_rx_buf[1];
            m_percent = (float(raw - (1 << 14)) / (1 << 15)) * 100.0f;
		}

        // polls the sensor for data
        void poll() override {
			if (m_mode == Mode::TX) {
				request_co2();
				m_mode = Mode::RX;
			} else if (m_mode == Mode::RX) {
				receive_buf();
				m_mode = Mode::TX;
			}
		}

        // attempts to initialize sensor, returns true on success and false on failure
        bool init() override {
			// Disable CRC (0x3768)
			uint8_t tx_buf1[2] = {0x37, 0x68};
            if (!m_smbus->blocking_transmit(CO2_ADDR, {reinterpret_cast<const char*>(tx_buf1), sizeof(tx_buf1)}))
				return false;

            // Set measurement mode -> standard measurement mode with 0-25% concentration in air
			uint8_t tx_buf2[4] = {0x36, 0x15, 0x00, 0x11};
            if (!m_smbus->blocking_transmit(CO2_ADDR, {reinterpret_cast<const char*>(tx_buf2), sizeof(tx_buf2)}))
				return false;

			m_mode = Mode::TX;

			return true;
		}
	};
}