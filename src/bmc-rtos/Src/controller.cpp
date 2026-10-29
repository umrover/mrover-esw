#include <MRoverCAN.hpp>
#include <hw/ad8418a.hpp>
#include <hw/hbridge.hpp>
#include <hw/limit_switch.hpp>
#include <hw/pin.hpp>
#include <hw/quadrature.hpp>
#include <logger.hpp>
#include <rtos/mutex.hpp>
#include <rtos/queue.hpp>
#include <rtos/semaphore.hpp>
#include <rtos/task.hpp>
#include <serial/fdcan.hpp>
#include <sys.hpp>
#include <timer.hpp>

#include "bmc-rtos_config.hpp"
#include "main.h"
#include "motor.hpp"
#include "type.hpp"


extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef hlpuart1;
extern FDCAN_HandleTypeDef hfdcan1;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim4;


namespace mrover {

    // params
    static constexpr uint8_t NUM_ADC_CHANNELS = 1;
    static constexpr size_t NUM_ELAPSED_TIMER_CHANNELS = 2;
    static constexpr size_t ELAPSED_TIMER_CH_1 = 0;
    static constexpr size_t ELAPSED_TIMER_CH_2 = 1;

    // peripheral handles
    static constexpr UART_HandleTypeDef* LPUART_1 = &hlpuart1;
    static constexpr ADC_HandleTypeDef* ADC_1 = &hadc1;
    static constexpr FDCAN_HandleTypeDef* FDCAN_1 = &hfdcan1;

    static constexpr TIM_HandleTypeDef* MOTOR_PWM_TIM = &htim1;
    static constexpr TIM_HandleTypeDef* ELAPSED_TIM = &htim2;
    static constexpr TIM_HandleTypeDef* ENCODER_TIM = &htim4;

    /*
     * define task periods, parameters
     * control - High - 40ms (25Hz) - PID and hbridge driving
     * limit-a/b - AboveNormal2 - edge/40ms (25Hz) - limit processing
     * can-rx - AboveNormal1 - per frame off buffer - drain frames into rx_queue
     * encoder - AboveNormal - 40ms (25Hz) - sampled 2ms ahead of control loop
     * current - AboveNormal - 40ms (25Hz) - sampled 2ms ahead of control loop
     * can-tx - Normal - 1 / tx_rate_hz - motor state broadcast
     */
    static constexpr uint32_t CONTROL_PERIOD_MS = 40;
    static constexpr uint32_t SENSOR_LEAD_MS = 2;
    static constexpr uint32_t LIMIT_DEBOUNCE_MS = 5;
    static constexpr uint32_t LIMIT_POLL_MS = CONTROL_PERIOD_MS;
    static constexpr uint32_t CAN_WWDG_MS = 100;
    static constexpr uint8_t DEFAULT_TX_RATE_HZ = 10;
    static constexpr uint8_t MAX_TX_RATE_HZ = 100;
    static constexpr std::size_t RX_QUEUE_DEPTH = 8;
    static constexpr uint32_t LIMIT_CHANGED = 1U << 0;

    // globals
    bmc_config_t config;
    bool volatile initialized = false;
    static uint8_t can_id{};
    static uint8_t host_can_id{};

    // peripherals
    std::optional<UART> lpuart;
    std::optional<ADC<NUM_ADC_CHANNELS>> adc;
    std::optional<FDCAN> fdcan;

    // timers
    std::optional<ElapsedTimer<NUM_ELAPSED_TIMER_CHANNELS>> elapsed_timer;

    // hardware units
    std::optional<Pin> can_tx;
    std::optional<Pin> can_rx;
    std::optional<MRoverCANHandler> can_receiver;
    std::optional<Motor> motor;

    // motor guard (for motor, config, and tick)
    static Mutex motor_lock{"motor"};
    static uint32_t last_rx_tick{};

    // incomingg message queue
    static Queue<std::optional<MRoverCANMsg_t>, RX_QUEUE_DEPTH> rx_queue{"can-rx"};
    static uint8_t volatile num_rx_dropped{}; // num dropped incoming frames

    // limit inputs
    struct limit_trigger_t {
        uint16_t pin;
        Semaphore edge{};
    };

    static limit_trigger_t limit_a_input{LIMIT_A_Pin};
    static limit_trigger_t limit_b_input{LIMIT_B_Pin};

    /**
     * Advance to next task release, keep phase if overrun.
     */
    static auto advance(uint32_t &next, uint32_t const period) -> void {
        next += period;
        while (static_cast<int32_t>(next - osKernelGetTickCount()) <= 0) next += period;
    }

    /**
     * Send a CAN message defined in MRoverCAN.dbc on the bus.
     * In RTOS application, this must be called by tasks.
     */
    auto send_can_message(uint32_t const base_id, uint8_t const* data, std::size_t const len) -> void {
        if (!initialized) return;
        can_tx->set();
        can_receiver->send_raw(base_id, data, len, can_id, host_can_id);
        can_tx->reset();
    }

    /**
     * Determine tx period from config params.
     */
    static auto tx_period_ms() -> uint32_t {
        uint8_t hz = config.get<bmc_config_t::tx_rate_hz>();
        if (hz == 0 || hz > MAX_TX_RATE_HZ) hz = DEFAULT_TX_RATE_HZ;
        return 1000 / hz;
    }

    // task function stubs
    static void control_task();
    static void limit_task(void* arg);
    static void rx_task();
    static void encoder_task();
    static void current_task();
    static void tx_task();

    // tasks
    static Task<1024> control{"control", control_task, osPriorityHigh};
    static Task<512> limit_a{"limit-a", limit_task, &limit_a_input, osPriorityAboveNormal2};
    static Task<512> limit_b{"limit-b", limit_task, &limit_b_input, osPriorityAboveNormal2};
    static Task<1024> rx{"can-rx", rx_task, osPriorityAboveNormal1};
    static Task<512> encoder{"encoder", encoder_task, osPriorityAboveNormal};
    static Task<512> current{"current", current_task, osPriorityAboveNormal};
    static Task<1024> tx{"can-tx", tx_task, osPriorityNormal};

    static auto control_task() -> void {
        uint32_t next = SENSOR_LEAD_MS;
        for (;;) {
            advance(next, CONTROL_PERIOD_MS);

            // wake on release or limit
            for (int32_t left; (left = static_cast<int32_t>(next - osKernelGetTickCount())) > 0;) {
                if (osThreadFlagsWait(LIMIT_CHANGED, osFlagsWaitAny, left) & osFlagsError) break;
                MutexGuard lock{motor_lock};
                motor->enforce_limits();
            }

            MutexGuard lock{motor_lock};
            if (osKernelGetTickCount() - last_rx_tick >= CAN_WWDG_MS) motor->tx_watchdog_lapsed();
            motor->drive_output();
        }
    }

    static auto limit_task(void* arg) -> void {
        auto& input = *static_cast<limit_trigger_t*>(arg);

        auto const update = [] {
            bool changed;
            {
                MutexGuard lock{motor_lock};
                changed = motor->update_limits();
            }
            if (changed) osThreadFlagsSet(control.handle(), LIMIT_CHANGED);
        };

        for (;;) {
            bool const edge = input.edge.acquire(LIMIT_POLL_MS);
            update();
            if (!edge) continue;

            // debounce limit pulse
            osDelay(LIMIT_DEBOUNCE_MS);
            update();
            __HAL_GPIO_EXTI_CLEAR_IT(input.pin);
            SET_BIT(EXTI->IMR1, input.pin);
        }
    }

    static auto rx_task() -> void {
        for (;;) {
            std::optional<MRoverCANMsg_t> msg;
            if (!rx_queue.get(msg) || !msg) continue;

            can_rx->set();
            {
                MutexGuard lock{motor_lock};
                last_rx_tick = osKernelGetTickCount();
                motor->reset_wwdg();
                motor->receive(*msg);
            }
            can_rx->reset();
        }
    }

    static auto encoder_task() -> void {
        uint32_t next = 0;
        for (;;) {
            advance(next, CONTROL_PERIOD_MS);
            osDelayUntil(next);
            MutexGuard lock{motor_lock};
            motor->update_encoder();
        }
    }

    static auto current_task() -> void {
        uint32_t next = 0;
        for (;;) {
            advance(next, CONTROL_PERIOD_MS);
            osDelayUntil(next);
            MutexGuard lock{motor_lock};
            motor->update_current();
        }
    }

    static auto tx_task() -> void {
        uint32_t next = 0;
        for (;;) {
            {
                MutexGuard lock{motor_lock};
                advance(next, tx_period_ms());
            }
            osDelayUntil(next);

            auto const state = [] {
                MutexGuard lock{motor_lock};
                return motor->state(num_rx_dropped);
            }();
            send_can_message(BMCMotorState::BASE_ID, state.msg_arr, sizeof(state.msg_arr));
        }
    }

    /**
     * Initialization sequence for BMC.
     */
    auto init() -> void {
        System::InterruptGuard guard{};
        System::get().init(get_sys_options());

        // initialize peripherals
        lpuart.emplace(LPUART_1, get_uart_options());
        adc.emplace(ADC_1, get_adc_options());
        fdcan.emplace(FDCAN_1, get_can_options(&config));

        // initialize logger
        Logger::init(&*lpuart);

        // setup timers
        elapsed_timer.emplace(ELAPSED_TIM, false); // pid compute timer

        // timer channels
        auto* pid_timer_handle = elapsed_timer->get_handle(ELAPSED_TIMER_CH_1);
        auto* enc_timer_handle = elapsed_timer->get_handle(ELAPSED_TIMER_CH_2);

        // setup debug LEDs
        can_tx.emplace(CAN_TX_LED_GPIO_Port, CAN_TX_LED_Pin);
        can_rx.emplace(CAN_RX_LED_GPIO_Port, CAN_RX_LED_Pin);

        // initialize fdcan (node ids are read once, as before)
        can_id = config.get<bmc_config_t::can_id>();
        host_can_id = config.get<bmc_config_t::host_can_id>();
        can_receiver = MRoverCANHandler{&*fdcan};

        // setup motor instance
        motor.emplace(
                HBridge{MOTOR_PWM_TIM, TIM_CHANNEL_1, Pin{MOTOR_DIR_GPIO_Port, MOTOR_DIR_Pin}},
                AD8418A{&*adc, ADC_CHANNEL_0},
                LimitSwitch{Pin{LIMIT_A_GPIO_Port, LIMIT_A_Pin}},
                LimitSwitch{Pin{LIMIT_B_GPIO_Port, LIMIT_B_Pin}},
                QuadratureEncoder{ENCODER_TIM, enc_timer_handle},
                send_can_message,
                pid_timer_handle,
                &config);

        // set initialization state and initial error state
        initialized = true;
    }

    /**
     * Callback enabling asynchronous serial logs via UART/DMA.
     * @param huart UART handle from callback
     */
    auto uart_tx_callback(UART_HandleTypeDef const* huart) -> void {
        if (huart == LPUART_1) {
            lpuart->handle_tx_complete();
        }
    }

} // namespace mrover

extern "C" {

void Init() {
    mrover::init();

    // init/start rtos kernel and tasks
    osKernelInitialize();
    mrover::control.start();
    mrover::limit_a.start();
    mrover::limit_b.start();
    mrover::rx.start();
    mrover::encoder.start();
    mrover::current.start();
    mrover::tx.start();
    osKernelStart();
}

// drain tx-fifo
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs) {
    if (!mrover::initialized) return;
    while (mrover::fdcan->messages_to_process() > 0) {
        if (auto const msg = mrover::can_receiver->receive(); msg && !mrover::rx_queue.put_from_isr(msg)) {
            // increment number dropped if we fail to enqueue a message
            mrover::num_rx_dropped++;
        }
    }
}

// mask line until debounced
void HAL_GPIO_EXTI_Callback(uint16_t const pin) {
    auto* input = pin == LIMIT_A_Pin ? &mrover::limit_a_input : pin == LIMIT_B_Pin ? &mrover::limit_b_input : nullptr;
    if (!input) return;
    CLEAR_BIT(EXTI->IMR1, pin);
    input->edge.release();
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
    mrover::uart_tx_callback(huart);
}

// TODO(eric) implement
void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef* hfdcan) {}
void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef* hfdcan, uint32_t ErrorStatusITs) {}
}
