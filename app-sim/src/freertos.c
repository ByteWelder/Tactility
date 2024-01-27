#include "tactility.h"
#include "thread.h"

#include "FreeRTOS.h"
#include "task.h"

#define TAG "freertos"

void app_main();

static void main_task(TT_UNUSED void* parameter) {
    TT_LOG_I(TAG, "starting app_main()");
    app_main();
    TT_LOG_I(TAG, "returned from app_main()");
    vTaskDelete(NULL);
}

int main() {
    BaseType_t task_result = xTaskCreate(
        main_task,
        "main",
        8192,
        NULL,
        ThreadPriorityNormal,
        NULL
    );

    tt_assert(task_result == pdTRUE);

    // Blocks forever
    vTaskStartScheduler();
}

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
