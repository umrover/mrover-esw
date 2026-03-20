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
        THPSensor thp_sensor{};
        CO2Sensor co2_sensor{};
        OzoneSensor ozone_sensor{};
        OxygenSensor oxygen_sensor{};
        UVSensor uv_sensor{};
        Pin can_tx{};
        Pin can_rx{};
        Pin dbg_led1{};
        Pin dbg_led2{};
        Pin dbg_led3{};
        MRoverCANHandler can_handler{};
        sb_config_t config;
        std::queue<sensor_t>* i2c_queue;
        ScienceSensor* i2c_sensors[NUM_I2C_SENSORS];

        // clears all sensor faults by resetting all bits to 1
        void clear_faults() {
            for (uint8_t i = 0; i < NUM_I2C_SENSORS; i++) {
                if (!i2c_sensors[i]->get_state()) {
                    i2c_queue->push(static_cast<sensor_t>(i));
                    i2c_sensors[i]->clear();
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
                std::queue<sensor_t>* i2c_queue_in) : thp_sensor(thp_in),
                                                    co2_sensor(co2_in),
                                                    ozone_sensor(ozone_in), 
                                                    oxygen_sensor(oxygen_in),
                                                    uv_sensor(uv_in),
                                                    can_tx(can_tx_in),
                                                    can_rx(can_rx_in),
                                                    dbg_led1(dbg_led1_in),
                                                    dbg_led2(dbg_led2_in),
                                                    dbg_led3(dbg_led3_in),
                                                    can_handler(can_handler_in),
                                                    i2c_queue(i2c_queue_in) {}

        // initialize i2c sensors
        void init() {
            i2c_sensors[static_cast<uint8_t>(sensor_t::sensor_co2)] = &co2_sensor;
            i2c_sensors[static_cast<uint8_t>(sensor_t::sensor_thp)] = &thp_sensor;
            i2c_sensors[static_cast<uint8_t>(sensor_t::sensor_oxygen)] = &oxygen_sensor;
            i2c_sensors[static_cast<uint8_t>(sensor_t::sensor_ozone)] = &ozone_sensor;

            for (uint8_t i = 0; i < NUM_I2C_SENSORS; i++) {
                if (!i2c_sensors[i]->init())
                    i2c_sensors[i]->flag();
                else
                    i2c_queue->push(static_cast<sensor_t>(i));
            }
        }

        // tries to reinitialize any sensors with an error state
        void restart_sensors() {
            auto& logger = Logger::instance();
            for (uint8_t i = 0; i < NUM_I2C_SENSORS; i++) {
                if (!i2c_sensors[i]->get_state()) {
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
                    if (i2c_sensors[i]->init()) {
                        i2c_queue->push(st);
                        i2c_sensors[i]->clear();
                        logger.info("%s restart successful", sensor_name.c_str());
                    } else {
                        logger.info("%s restart failed", sensor_name.c_str());
                    }
                }
            }
        }

        bool check_sensor (sensor_t st) {
            if (st == sensor_t::sensor_uv)
                return uv_sensor.get_state();
            else
                return i2c_sensors[static_cast<uint8_t>(st)]->get_state();
        }

        void update_sensor (sensor_t st) {
            if (st == sensor_t::sensor_uv)
                uv_sensor.update();
            else
                i2c_sensors[static_cast<uint8_t>(st)]->update();
        }

        void poll_sensor (sensor_t st) {
            if (st == sensor_t::sensor_uv)
                uv_sensor.poll();
            else
                i2c_sensors[static_cast<uint8_t>(st)]->poll();
        }

        void flag_sensor (sensor_t st) {
            if (st == sensor_t::sensor_uv)
                uv_sensor.flag();
            else
                i2c_sensors[static_cast<uint8_t>(st)]->flag();
        }

        void send_sensor_data() {
            can_tx.set();
            const MRoverCANMsg_t msg = SCISensorData(
                                                uv_sensor.get_uv(), 
                                                thp_sensor.get_thp().temp, 
                                                thp_sensor.get_thp().humidity, 
                                                thp_sensor.get_thp().pressure, 
                                                oxygen_sensor.get_oxygen(), 
                                                ozone_sensor.get_ozone(),
                                                co2_sensor.get_co2());
            can_handler.send(msg, 0x40, 0x10);
            can_tx.reset();
        }

        void send_sensor_state() {
            can_tx.set();
            const MRoverCANMsg_t msg = SCISensorState(uv_sensor.get_state(), 
                                                    thp_sensor.get_state(),
                                                    oxygen_sensor.get_state(), 
                                                    ozone_sensor.get_state(), 
                                                    co2_sensor.get_state());
            can_handler.send(msg, 0x40, 0x10);
            can_tx.reset();
        }

        void handle_request() {
            auto const recv = can_handler.receive();
            if (recv) {
                can_rx.set();
                std::visit([this](auto&& value) { handle(value); }, *recv);
                can_rx.reset();
            }
        }
    };
}