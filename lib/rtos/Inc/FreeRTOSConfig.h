#pragma once

#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
#include <stdint.h>
extern uint32_t SystemCoreClock;
#endif

#define CMSIS_device_header "stm32g4xx.h"

/* Scheduler */
#define configUSE_PREEMPTION 1
#define configCPU_CLOCK_HZ (SystemCoreClock)
#define configTICK_RATE_HZ ((TickType_t) 1000)
/*
 * 56 priorities and no CLZ-based task selection are both hard requirements of
 * the CMSIS-RTOS v2 wrapper - osPriority_t spans 56 levels, while the Cortex-M
 * port optimisation only handles 32. freertos_os2.h #errors otherwise.
 */
#define configMAX_PRIORITIES (56)
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configMINIMAL_STACK_SIZE ((uint16_t) 128)
#define configMAX_TASK_NAME_LEN (16)
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1

#define configSUPPORT_STATIC_ALLOCATION 1
#define configSUPPORT_DYNAMIC_ALLOCATION 0

/* Synchronization primitives */
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 1
#define configUSE_COUNTING_SEMAPHORES 1
#define configUSE_TASK_NOTIFICATIONS 1
#define configQUEUE_REGISTRY_SIZE 8

#define configCHECK_FOR_STACK_OVERFLOW 2
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configUSE_MALLOC_FAILED_HOOK 0
#define configUSE_DAEMON_TASK_STARTUP_HOOK 0

#define configUSE_TIMERS 0
#define configTIMER_TASK_STACK_DEPTH 256
#define configUSE_OS2_EVENTFLAGS_FROM_ISR 0
#define configUSE_CO_ROUTINES 0
#define configUSE_TRACE_FACILITY 1
#define configUSE_STATS_FORMATTING_FUNCTIONS 0
#define configGENERATE_RUN_TIME_STATS 0
#define configUSE_TICKLESS_IDLE 0
#define configENABLE_BACKWARD_COMPATIBILITY 0

#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_uxTaskPriorityGet 1
#define INCLUDE_vTaskDelete 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_eTaskGetState 1
#define INCLUDE_xQueueGetMutexHolder 1
#define INCLUDE_xTimerPendFunctionCall 0
#define INCLUDE_vTaskCleanUpResources 0

#ifdef __NVIC_PRIO_BITS
#define configPRIO_BITS __NVIC_PRIO_BITS
#else
#define configPRIO_BITS 4
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define configASSERT(x)           \
    if ((x) == 0) {               \
        taskDISABLE_INTERRUPTS(); \
        for (;;);                 \
    }

#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
