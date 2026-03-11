#pragma once

#include "CO2Sensor.hpp"
#include "OxygenSensor.hpp"
#include "OzoneSensor.hpp"
#include "UVSensor.hpp"
#include "THPSensor.hpp"
#include <CANBus1.hpp>
#include <hw/pin.hpp>
#include <config.hpp>

namespace mrover {
    enum Sensor {
        sensor_co2_rx = 0,
        sensor_co2_tx = 1,
        sensor_thp = 2,
        sensor_ozone = 3,
        sensor_oxygen = 4,
        sensor_uv = 5,
    };

    // force into single byte
    #pragma pack(push,1)
    struct SensorStates {
        uint8_t uv_state : 1;
        uint8_t thp_state : 1;
        uint8_t oxygen_state : 1;
        uint8_t ozone_state : 1;
        uint8_t co2_state : 1;
    };
    #pragma pack(pop)

    class ScienceBoard {
    private:
        THP thp_sensor{};
        CO2Sensor co2_sensor{};
        OzoneSensor ozone_sensor{};
        OxygenSensor oxygen_sensor{};
        UVSensor uv_sensor{};
        Pin can_tx{};
        Pin can_rx{};
        Pin dbg_led1{};
        Pin dbg_led2{};
        Pin dbg_led3{};
        CANBus1Handler can_handler{};
        sb_config_t config;
        SensorStates sensor_states{0,0,0,0,0};

        void init() {
            // initialize i2c sensors
            thp_sensor.init();
            ozone_sensor.init();
            oxygen_sensor.init();
            co2_sensor.init();
        }

        void clear_faults() {
            sensor_states = {0,0,0,0,0};
        }

        void reset() {
            // deinitialize peripherals and reset mcu
            HAL_DeInit();
            NVIC_SystemReset();
        }

    public:
        ScienceBoard() = default;

        ScienceBoard(
                THP& thp_in, 
                CO2Sensor& co2_in, 
                OzoneSensor& ozone_in,
                OxygenSensor& oxygen_in,
                UVSensor& uv_in,
                Pin& can_tx_in,
                Pin& can_rx_in,
                Pin& dbg_led1_in,
                Pin& dbg_led2_in,
                Pin& dbg_led3_in,
                CANBus1Handler& can_handler_in) : thp_sensor(thp_in),
                                                    co2_sensor(co2_in),
                                                    ozone_sensor(ozone_in), 
                                                    oxygen_sensor(oxygen_in),
                                                    uv_sensor(uv_in),
                                                    can_tx(can_tx_in),
                                                    can_rx(can_rx_in),
                                                    dbg_led1(dbg_led1_in),
                                                    dbg_led2(dbg_led2_in),
                                                    dbg_led3(dbg_led3_in),
                                                    can_handler(can_handler_in) {
            init();
        }

        void update_sensor (Sensor sensor) {
            if (sensor == sensor_co2_rx)
                co2_sensor.update_co2();
            else if (sensor == sensor_thp)
                thp_sensor.update_thp();
            else if (sensor == sensor_oxygen)
                oxygen_sensor.update_oxygen();
            else if (sensor == sensor_ozone)
                ozone_sensor.update_ozone();
        }

        void poll_sensor (Sensor sensor) {
            if (sensor == sensor_co2_tx)
                co2_sensor.request_co2();
            else if (sensor == sensor_co2_rx)
                co2_sensor.receive_buf();
            else if (sensor == sensor_thp)
                thp_sensor.read_thp();
            else if (sensor == sensor_oxygen)
                oxygen_sensor.read_oxygen();
            else if (sensor == sensor_ozone)
                ozone_sensor.read_ozone();
            else if (sensor == sensor_uv)
                uv_sensor.sample_sensor();
        }

        void flag_sensor (Sensor sensor) {
            if (sensor == sensor_co2_tx || sensor_co2_rx)
                sensor_states.co2_state = 1;
            else if (sensor == sensor_thp)
                sensor_states.thp_state = 1;
            else if (sensor == sensor_oxygen)
                sensor_states.oxygen_state = 1;
            else if (sensor == sensor_ozone)
                sensor_states.ozone_state = 1;
            else if (sensor == sensor_uv)
                sensor_states.uv_state = 1;
        }

        void send_sensor_data() {
            can_tx.set();
            const CANBus1Msg_t msg = SCISensorData(
                                                uv_sensor.get_current_uv(), 
                                                thp_sensor.get_thp().temp, 
                                                thp_sensor.get_thp().humidity, 
                                                thp_sensor.get_thp().pressure, 
                                                oxygen_sensor.get_oxygen(), 
                                                ozone_sensor.get_ozone(),
                                                co2_sensor.get_co2());
            // can_handler.send(msg, config.get<sb_config_t::can_id>(), config.get<sb_config_t::host_can_id>());
            can_handler.send(msg, 0x40, 0x10);
            can_tx.reset();
        }

        void send_sensor_state() {
            can_tx.set();
            const CANBus1Msg_t msg = SCISensorState(sensor_states.uv_state, 
                                                    sensor_states.thp_state,
                                                    sensor_states.oxygen_state, 
                                                    sensor_states.ozone_state, 
                                                    sensor_states.co2_state);
            // can_handler.send(msg, config.get<sb_config_t::can_id>(), config.get<sb_config_t::host_can_id>());
            can_handler.send(msg, 0x40, 0x10);
            can_tx.reset();
        }
    };
}