#include <optional>

#include <MRoverCAN.hpp>
#include <hw/pin.hpp>
#include <logger.hpp>
#include <rtos/semaphore.hpp>
#include <rtos/task.hpp>
#include <serial/fdcan.hpp>
#include <serial/uart.hpp>
#include <sys.hpp>

#include "canprobe_config.hpp"
#include "main.h"
#include "probe.hpp"

extern UART_HandleTypeDef hlpuart1;
extern FDCAN_HandleTypeDef hfdcan1;

namespace mrover {

    static constexpr uint32_t PROBE_PERIOD_MS = 1000;

    canprobe_config_t config;

    std::optional<UART> lpuart;
    std::optional<FDCAN> fdcan;
    std::optional<MRoverCANHandler> can_receiver;

    std::optional<Pin> can_stb;

    std::optional<ProbeHandler> probe_handler;

    static uint8_t can_id = 0;
    static uint8_t host_can_id = 0;

    static Semaphore rx_ready{};

    static auto send_can_message(uint32_t const base_id, uint8_t const* data, std::size_t const len) -> void {
        can_receiver->send_raw(base_id, data, len, can_id, host_can_id);
    }

    static void rx_task();
    static void tx_task();

    static Task<1024> rx{"can-rx", rx_task, osPriorityAboveNormal};
    static Task<1024> tx{"can-tx", tx_task, osPriorityLow};

    static void rx_task() {
        for (;;) {
            if (!rx_ready.acquire()) continue;

            while (fdcan->messages_to_process() > 0) {
                auto const recv = can_receiver->receive();
                if (!recv) break;

                probe_handler->receive(*recv);
            }
        }
    }

    static void tx_task() {
        for (;;) {
            probe_handler->send_probe();
            osDelay(PROBE_PERIOD_MS);
        }
    }

    static auto init() -> void {
        System::get().init();

        can_stb.emplace(CAN_STB_GPIO_Port, CAN_STB_Pin);
        can_stb->reset();

        lpuart.emplace(&hlpuart1);
        Logger::init(&*lpuart, Logger::Level::Debug);

        can_id = config.get<canprobe_config_t::can_id>();
        host_can_id = config.get<canprobe_config_t::host_can_id>();
        Logger::instance().info("canprobe up: can_id=%u host_can_id=%u", can_id, host_can_id);

        fdcan.emplace(&hfdcan1, get_can_options(&config));
        can_receiver.emplace(&*fdcan);

        probe_handler.emplace(send_can_message, &config);
    }

} // namespace mrover

extern "C" {

void Init() {
    mrover::init();

    osKernelInitialize();
    mrover::rx.start();
    mrover::tx.start();
    osKernelStart();
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t const RxFifo0ITs) {
    if (hfdcan == &hfdcan1 && (RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE)) {
        mrover::rx_ready.release();
    }
}

} // extern "C"
