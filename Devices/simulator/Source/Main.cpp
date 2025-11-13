#include "Main.h"
#include <Tactility/TactilityCore.h>
#include <Tactility/Thread.h>

#include "FreeRTOS.h"
#include "task.h"

#define TAG "freertos"

namespace simulator {

MainFunction mainFunction = nullptr;

void setMain(MainFunction newMainFunction) {
    mainFunction = newMainFunction;
}

static void freertosMainTask(TT_UNUSED void* parameter) {
    TT_LOG_I(TAG, "starting app_main()");
    assert(simulator::mainFunction);
    mainFunction();
    TT_LOG_I(TAG, "returned from app_main()");
    vTaskDelete(nullptr);
}

void freertosMain() {
    BaseType_t task_result = xTaskCreate(
        freertosMainTask,
        "main",
        8192,
        nullptr,
        static_cast<UBaseType_t>(tt::Thread::Priority::Normal),
        nullptr
    );

    assert(task_result == pdTRUE);

    // Blocks forever
    vTaskStartScheduler();
}

} // namespace

/**
 * Assert implementation as defined in the FreeRTOSConfig.h
 * It allows you to set breakpoints and debug asserts.
 */
void vAssertCalled(unsigned long line, const char* const file) {
    static portBASE_TYPE xPrinted = pdFALSE;
    volatile uint32_t set_to_nonzero_in_debugger_to_continue = 0;

    TT_LOG_E(TAG, "assert triggered at %s:%d", file, line);
    taskENTER_CRITICAL();
    {
        // Step out by attaching a debugger and setting set_to_nonzero_in_debugger_to_continue
        while (set_to_nonzero_in_debugger_to_continue == 0) {
            // NO-OP
        }
    }
    taskEXIT_CRITICAL();
}

