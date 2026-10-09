#include "main.h"

#include <cmath>

#include <cordic.hpp>
#include <serial/uart.hpp>
#include <sys.hpp>
#include <vcp.hpp>


extern UART_HandleTypeDef hlpuart1;
extern CORDIC_HandleTypeDef hcordic;


namespace mrover {

    // (names avoid the bare CORDIC/FLASH tokens, which CMSIS #defines)
    enum class config_t : uint32_t {
        TEST_CORDIC = 0,
        TEST_SOFTWARE = 1,
    };

    struct pipeline_request_t {
        config_t config;
        abc_t i_abc; // measured phase currents
        float theta; // rotor electrical angle, radians
        dq_t i_dq_ref; // current references
    };

    struct pipeline_response_t {
        uint32_t cycles; // DWT cycles for the step alone
        dq_t i_dq; // Park output (measured d/q currents)
        abc_t v_abc; // inverse Clarke output (phase voltage commands)
    };

    static_assert(sizeof(pipeline_request_t) == 28 && sizeof(pipeline_response_t) == 24);

    constexpr float KP = 0.5f; // placeholder

    // peripherals
    UART lpuart;
    CORDICDriver cordic;

    // components
    VCP vcp;

    template<config_t C>
    auto sincos(float const theta) -> sincos_t {
        if constexpr (C == config_t::TEST_CORDIC) {
            return cordic.sincos(theta);
        } else {
            return {std::sin(theta), std::cos(theta)};
        }
    }

    template<config_t C>
    auto foc_step(pipeline_request_t const& req, pipeline_response_t& resp) -> void {
        auto const i_ab = CORDICDriver::clarke(req.i_abc);
        auto const sc = sincos<C>(req.theta);
        resp.i_dq = CORDICDriver::park(i_ab, sc);
        // placeholder P controller
        dq_t const v_dq{KP * (req.i_dq_ref.d - resp.i_dq.d), KP * (req.i_dq_ref.q - resp.i_dq.q)};
        resp.v_abc = CORDICDriver::inv_clarke(CORDICDriver::inv_park(v_dq, sc));
    }

    CCMSRAM [[gnu::flatten]] auto test_cordic(pipeline_request_t const& req) -> pipeline_response_t {
        pipeline_response_t resp;
        resp.cycles = System::profile([&] { foc_step<config_t::TEST_CORDIC>(req, resp); });
        return resp;
    }

    [[gnu::noinline, gnu::flatten]] auto test_software(pipeline_request_t const& req) -> pipeline_response_t {
        pipeline_response_t resp;
        resp.cycles = System::profile([&] { foc_step<config_t::TEST_SOFTWARE>(req, resp); });
        return resp;
    }

    auto get_uart_options() -> UART::Options {
        UART::Options options;
        options.use_tx_interrupt = true;
        options.use_rx_interrupt = true;
        return options;
    }

    [[noreturn]] auto init() -> void {
        System::get().init();
        System::init_ccmsram();
        lpuart = UART{&hlpuart1, get_uart_options()};
        cordic = CORDICDriver{&hcordic};
        vcp = VCP{&lpuart};

        for (;;) {
            pipeline_request_t req;
            if (!vcp.receive(req)) continue;
            switch (req.config) {
                case config_t::TEST_CORDIC:
                    vcp.send(test_cordic(req));
                    break;
                case config_t::TEST_SOFTWARE:
                    vcp.send(test_software(req));
                    break;
            }
        }
    }

} // namespace mrover

extern "C" {

void Init() {
    mrover::init();
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
    if (huart == &hlpuart1) mrover::lpuart.handle_tx_complete();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
    if (huart == &hlpuart1) mrover::lpuart.handle_rx_complete();
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
    if (huart == &hlpuart1) mrover::lpuart.handle_error();
}

}
