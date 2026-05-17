#pragma once

#include "CO2Sensor.hpp"
#include "OxygenSensor.hpp"
#include "OzoneSensor.hpp"
#include "UVSensor.hpp"
#include "THPSensor.hpp"
#include <MRoverCAN.hpp>
#include <hw/pin.hpp>
#include <config.hpp>
#include <logger.hpp>
#include <queue>
#include <string>

namespace mrover {
    enum class sensor_t {
        sensor_co2 = 0,
        sensor_thp = 1,
        sensor_ozone = 2,
        sensor_oxygen = 3,
        sensor_uv = 4,
    };

    const size_t NUM_I2C_SENSORS = 4;

    class ScienceBoard {
    private:
        THPSensor m_thp_sensor{};
        CO2Sensor m_co2_sensor{};
        OzoneSensor m_ozone_sensor{};
        OxygenSensor m_oxygen_sensor{};
        UVSensor m_uv_sensor{};
        Pin m_can_tx{};
        Pin m_can_rx{};
        Pin m_dbg_led1{};
        Pin m_dbg_led2{};
        Pin m_dbg_led3{};
        MRoverCANHandler m_can_handler{};
        std::queue<sensor_t>* m_i2c_queue;
        ScienceSensor* m_i2c_sensors[NUM_I2C_SENSORS];

        // clears all sensor faults by resetting all bits to 1
        void clear_faults() {
            for (uint8_t i = 0; i < NUM_I2C_SENSORS; i++) {
                if (!m_i2c_sensors[i]->get_state()) {
                    m_i2c_queue->push(static_cast<sensor_t>(i));
                    m_i2c_sensors[i]->clear();
                }
            }
        }

        // deinitialize peripherals and reset mcu
        static void reset() {
            HAL_DeInit();
            NVIC_SystemReset();
        }

        // need empty function for any unrelated messages
        template<typename T>
        void handle(T const& _) {
        }

        // handles a reset command
        void handle(const SCIResetCommand& cmd) {
            if (cmd.clear_faults)
                clear_faults();
            else if (cmd.reset)
                reset();
        }

    public:
        ScienceBoard() = default;

        ScienceBoard(
                THPSensor& thp_in, 
                CO2Sensor& co2_in, 
                OzoneSensor& ozone_in,
                OxygenSensor& oxygen_in,
                UVSensor& uv_in,
                Pin& can_tx_in,
                Pin& can_rx_in,
                Pin& dbg_led1_in,
                Pin& dbg_led2_in,
                Pin& dbg_led3_in,
                MRoverCANHandler& can_handler_in,
                std::queue<sensor_t>* i2c_queue_in) : m_thp_sensor(thp_in),
                                                    m_co2_sensor(co2_in),
                                                    m_ozone_sensor(ozone_in), 
                                                    m_oxygen_sensor(oxygen_in),
                                                    m_uv_sensor(uv_in),
                                                    m_can_tx(can_tx_in),
                                                    m_can_rx(can_rx_in),
                                                    m_dbg_led1(dbg_led1_in),
                                                    m_dbg_led2(dbg_led2_in),
                                                    m_dbg_led3(dbg_led3_in),
                                                    m_can_handler(can_handler_in),
                                                    m_i2c_queue(i2c_queue_in) {}

        // initialize i2c sensors
        void init() {
            m_i2c_sensors[static_cast<uint8_t>(sensor_t::sensor_co2)] = &m_co2_sensor;
            m_i2c_sensors[static_cast<uint8_t>(sensor_t::sensor_thp)] = &m_thp_sensor;
            m_i2c_sensors[static_cast<uint8_t>(sensor_t::sensor_oxygen)] = &m_oxygen_sensor;
            m_i2c_sensors[static_cast<uint8_t>(sensor_t::sensor_ozone)] = &m_ozone_sensor;

            for (uint8_t i = 0; i < NUM_I2C_SENSORS; i++) {
                if (!m_i2c_sensors[i]->init())
                    m_i2c_sensors[i]->flag();
                else
                    m_i2c_queue->push(static_cast<sensor_t>(i));
            }
        }

        // tries to reinitialize any sensors with an error state
        void restart_sensors() {
            auto& logger = Logger::instance();
            for (uint8_t i = 0; i < NUM_I2C_SENSORS; i++) {
                if (!m_i2c_sensors[i]->get_state()) {
                    std::string sensor_name;
                    sensor_t st = static_cast<sensor_t>(i);

                    if (st == sensor_t::sensor_co2)
                        sensor_name = "CO2";
                    else if (st == sensor_t::sensor_thp)
                        sensor_name = "THP";
                    else if (st == sensor_t::sensor_oxygen)
                        sensor_name = "Oxygen";
                    else if (st == sensor_t::sensor_ozone)
                        sensor_name = "Ozone";

                    logger.info("Attempting to restart %s...", sensor_name.c_str());
                    if (m_i2c_sensors[i]->init()) {
                        m_i2c_queue->push(st);
                        m_i2c_sensors[i]->clear();
                        logger.info("%s restart successful!", sensor_name.c_str());
                    } else {
                        logger.info("%s restart failed!", sensor_name.c_str());
                    }
                }
            }
        }

        bool check_sensor (sensor_t st) {
            if (st == sensor_t::sensor_uv)
                return m_uv_sensor.get_state();
            else
                return m_i2c_sensors[static_cast<uint8_t>(st)]->get_state();
        }

        void update_sensor (sensor_t st) {
            if (st == sensor_t::sensor_uv)
                m_uv_sensor.update();
            else
                m_i2c_sensors[static_cast<uint8_t>(st)]->update();
        }

        void poll_sensor (sensor_t st) {
            if (st == sensor_t::sensor_uv)
                m_uv_sensor.poll();
            else
                m_i2c_sensors[static_cast<uint8_t>(st)]->poll();
        }

        void flag_sensor (sensor_t st) {
            if (st == sensor_t::sensor_uv)
                m_uv_sensor.flag();
            else
                m_i2c_sensors[static_cast<uint8_t>(st)]->flag();
        }

        void send_sensor_data() {
            m_can_tx.set();
            const MRoverCANMsg_t msg = SCISensorData(
                                                m_uv_sensor.get_uv(), 
                                                m_thp_sensor.get_thp().temp, 
                                                m_thp_sensor.get_thp().humidity, 
                                                m_thp_sensor.get_thp().pressure, 
                                                m_oxygen_sensor.get_oxygen(), 
                                                m_ozone_sensor.get_ozone(),
                                                m_co2_sensor.get_co2());
            m_can_handler.send(msg, SB_CAN_ID, JETSON_CAN_ID);
            m_can_tx.reset();
        }

        void send_sensor_state() {
            m_can_tx.set();
            const MRoverCANMsg_t msg = SCISensorState(m_uv_sensor.get_state(), 
                                                    m_thp_sensor.get_state(),
                                                    m_oxygen_sensor.get_state(), 
                                                    m_ozone_sensor.get_state(), 
                                                    m_co2_sensor.get_state());
            m_can_handler.send(msg, SB_CAN_ID, JETSON_CAN_ID);
            m_can_tx.reset();
        }

        void handle_request() {
            auto const recv = m_can_handler.receive();
            if (recv) {
                m_can_rx.set();
                std::visit([this](auto&& value) { handle(value); }, *recv);
                m_can_rx.reset();
            }
        }
    };
}