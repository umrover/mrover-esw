#pragma once

#include <bit>
#include <cstdint>
#include <tuple>
#include <type_traits>


namespace mrover {
    // retrieves science board uart options
    inline auto get_uart_options() -> UART::Options {
        UART::Options options;
        options.use_dma = false;
        return options;
    }

    // retrieves science board can options
    // inline auto get_can_options() -> FDCAN::Options {
    //     auto can_opts = FDCAN::Options{};
    //     can_opts.delay_compensation = true;
    //     can_opts.tdc_offset = 13;
    //     can_opts.tdc_filter = 1;
    //     return can_opts;
    // }
} // namespace mrover
