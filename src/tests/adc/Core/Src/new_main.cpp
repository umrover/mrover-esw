#include "main.h"

#include <adc.hpp>
#include <logger.hpp>
#include <serial/uart.hpp>
#include <sys.hpp>

extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef hlpuart1;

namespace mrover {

    UART lpuart;

    [[noreturn]] auto init() -> void {
        lpuart = UART{&hlpuart1, UART::Options{}};
        Logger::init(&lpuart);
        auto& log = Logger::instance();

        static ADC adc{&hadc1, ADC::options_t{
                                       .mode = ADC::mode_t::DMA,
                                       .self_calibrate = true,
                                       .timeout_ms = 10,
                               }};
        if (!adc.init()) {
            log.error("adc init failed");
            System::fault();
        }

        for (;;) {
            log.info("%u %u %u %u %u %u %u %u", adc.value(0), adc.value(1), adc.value(2), adc.value(3),
                     adc.value(4), adc.value(5), adc.value(6), adc.value(7));
            System::delay_ms(500);
        }
    }

} // namespace mrover

extern "C" {

void PostInit() {
    mrover::init();
}
}
