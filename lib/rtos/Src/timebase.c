#include "main.h"

extern uint32_t uwTickPrio;

#define RTOS_TIMEBASE_HZ 1000U
#define RTOS_TIMEBASE_COUNTER_HZ 1000000U

static uint32_t rtos_tim6_clock(void) {
    RCC_ClkInitTypeDef clk_config = {0};
    uint32_t flash_latency = 0;

    HAL_RCC_GetClockConfig(&clk_config, &flash_latency);

    if (clk_config.APB1CLKDivider == RCC_HCLK_DIV1) {
        return HAL_RCC_GetPCLK1Freq();
    }
    return 2UL * HAL_RCC_GetPCLK1Freq();
}

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
    if (TickPriority >= (1UL << __NVIC_PRIO_BITS)) {
        return HAL_ERROR;
    }

    uint32_t const timer_clock = rtos_tim6_clock();
    if (timer_clock < RTOS_TIMEBASE_COUNTER_HZ) {
        return HAL_ERROR;
    }

    __HAL_RCC_TIM6_CLK_ENABLE();

    TIM6->CR1 = 0;
    TIM6->PSC = (timer_clock / RTOS_TIMEBASE_COUNTER_HZ) - 1U;
    TIM6->ARR = (RTOS_TIMEBASE_COUNTER_HZ / RTOS_TIMEBASE_HZ) - 1U;
    TIM6->EGR = TIM_EGR_UG;
    TIM6->SR = 0;
    TIM6->DIER = TIM_DIER_UIE;
    TIM6->CR1 = TIM_CR1_CEN;

    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, TickPriority, 0U);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    uwTickPrio = TickPriority;

    return HAL_OK;
}

void HAL_SuspendTick(void) {
    TIM6->DIER &= ~TIM_DIER_UIE;
}

void HAL_ResumeTick(void) {
    TIM6->DIER |= TIM_DIER_UIE;
}

void TIM6_DAC_IRQHandler(void) {
    TIM6->SR = ~TIM_SR_UIF;
    HAL_IncTick();
}
