#pragma once

#include "serial/smbus.hpp"
#include "stm32g4xx_hal_def.h"
#include <MRoverCAN.hpp>
#include <adc.hpp>
#include <serial/fdcan.hpp>
#include <serial/uart.hpp>


namespace mrover {
    // CAN IDs
    uint8_t const PDLB_CAN_ID = 0x11;
    uint8_t const JETSON_CAN_ID = 0x10;

    // retrieves science pdlb can options
    inline auto get_can_options() -> FDCAN::Options {
        auto can_opts = FDCAN::Options{};
        can_opts.delay_compensation = true;
        can_opts.tdc_offset = 13;
        can_opts.tdc_filter = 1;
        return can_opts;
    }
} // namespace mrover
