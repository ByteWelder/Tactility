#include "tactility.h"

#include "FreeRTOS.h"
#include "task.h"

#define TAG "freertos"

#define mainQUEUE_RECEIVE_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

_Noreturn void app_main();

bool lvgl_is_ready();
void lvgl_task(void*);

void app_main_task(TT_UNUSED void* parameter) {
    while (!lvgl_is_ready()) {
        TT_LOG_I(TAG, "waiting for lvgl task");
        vTaskDelay(50);
    }

    app_main();
}

int main() {
    // Create the main app loop, like ESP-IDF
    xTaskCreate(
        lvgl_task,
        "lvgl",
        8192,
        NULL,
        mainQUEUE_RECEIVE_TASK_PRIORITY + 2,
        NULL
    );

    xTaskCreate(
        app_main_task,
        "app_main",
        8192,
        NULL,
        mainQUEUE_RECEIVE_TASK_PRIORITY + 1,
        NULL
    );

    // Blocks forever
    vTaskStartScheduler();
}

/**
 * Assert implementation as defined in the FreeRTOSConfig.h
 * It allows you to set breakpoints and debug asserts.
 */
void vAssertCalled(TT_UNUSED unsigned long line, TT_UNUSED const char* const file) {
    static portBASE_TYPE xPrinted = pdFALSE;
    volatile uint32_t set_to_nonzero_in_debugger_to_continue = 0;

    TT_LOG_E(TAG, "assert triggered");
    taskENTER_CRITICAL();
    {
        // Step out by attaching a debugger and setting set_to_nonzero_in_debugger_to_continue
        while (set_to_nonzero_in_debugger_to_continue == 0) {
            // NO-OP
        }
    }
    taskEXIT_CRITICAL();
}
