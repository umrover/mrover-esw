#pragma once

#include <adc.hpp>
#include <sys.hpp>
#include <hw/ad8418a.hpp>
#include <hw/flash.hpp>
#include <serial/uart.hpp>

namespace mrover {

    enum struct mode_t : uint8_t {
        STOPPED = 0,
        FAULT = 1,
        THROTTLE = 5,
        POSITION = 6,
        VELOCITY = 7,
    };

    enum struct encoder_mode_t : uint8_t {
        NONE = 0,
        QUAD,
    };

    /**
     * Get the BMC UART settings.
     *
     * DMA must be enabled to allow non-blocking logging functionality.
     *
     * @return UART options for BMC
     */
    inline auto get_uart_options() -> UART::Options {
        UART::Options options;
        options.use_dma = true;
        return options;
    }

    /**
     * Get the BMC ADC settings.
     *
     * DMA must be enabled to allow non-blocking ADC functionality.
     *
     * @return ADC options for BMC
     */
    inline auto get_adc_options() -> ADCBase::Options {
        ADCBase::Options options;
        options.use_dma = false; // TODO(eric) use dma for this
        return options;
    }

    inline auto get_current_sensor_options() -> AD8418A::Options {
        AD8418A::Options options;
        options.gain = 20.0f;
        options.shunt_resistance = 0.0125f;
        options.vref = 3.3f;
        options.vcm = 1.598f;
        options.adc_resolution = 4095;
        return options;
    }

    inline auto get_sys_options() -> System::options_t {
        System::options_t options;
        options.sleep_on_wfi = true;
        return options;
    }

} // namespace mrover
