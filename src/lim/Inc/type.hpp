#pragma once

#include <hw/flash.hpp>
#include <serial/uart.hpp>
#include <sys.hpp>

namespace mrover {
    inline auto get_uart_options() -> UART::Options {
        UART::Options options;
        options.use_dma = true;
        return options;
    }

    inline auto get_sys_options() -> System::options_t {
        System::options_t options;
        options.sleep_on_wfi = true;
        return options;
    }
} // namespace mrover
