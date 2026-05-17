#pragma once

#include "serial/smbus.hpp"
#include "stm32g4xx_hal_def.h"
#include <serial/fdcan.hpp>
#include <serial/uart.hpp>
#include <MRoverCAN.hpp>
#include <adc.hpp>


namespace mrover {
    // CAN IDs
    const uint8_t SB_CAN_ID = 0x40;
    const uint8_t JETSON_CAN_ID = 0x10;

    // retrieves science board uart options
    inline auto get_uart_options() -> UART::Options {
        UART::Options options;
        options.use_dma = false;
        return options;
    }

    // retrieves science board can options
    inline auto get_can_options() -> FDCAN::Options {
        auto can_opts = FDCAN::Options{};
        can_opts.delay_compensation = true;
        can_opts.tdc_offset = 13;
        can_opts.tdc_filter = 1;
        return can_opts;
    }

    // retrieves science board adc options
    inline auto get_adc_options() -> ADCBase::Options {
        ADCBase::Options options;
        options.use_dma = true;
        return options;
    }

    // retrieves science board i2c options
    inline auto get_smbus_options() -> SMBus::Options {
        SMBus::Options options;
        options.timeout_ms = HAL_MAX_DELAY;
        options.use_dma = true;
        options.enable_wd = true;
        return options;
    }
} // namespace mrover
