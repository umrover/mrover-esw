#include <FreeRTOS.h>
#include <task.h>

#include <sys.hpp>

extern "C" {

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    (void) xTask;
    (void) pcTaskName;

    mrover::System::fault(mrover::System::fault_reason_t::STACK_OVERFLOW);
}

} // extern "C"
