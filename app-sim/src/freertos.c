#include "tactility.h"

#include "FreeRTOS.h"
#include "task.h"

#define TAG "freertos"

#define mainQUEUE_RECEIVE_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

_Noreturn void app_main(void*);

extern void lvgl_init();
extern void lvgl_task(void*);

int main() {
    // Create the main app loop, like ESP-IDF
    xTaskCreate(
        lvgl_task,
        "lvgl",
        8192, // TODO: Reconsider a larger heap?
        NULL,
        mainQUEUE_RECEIVE_TASK_PRIORITY + 2,
        NULL
    );

    xTaskCreate(
        app_main,
        "app_main",
        8192, // TODO: Reconsider a larger heap?
        NULL,
        mainQUEUE_RECEIVE_TASK_PRIORITY + 1,
        NULL
    );

    // Blocks forever
    vTaskStartScheduler();
}

void vApplicationMallocFailedHook() {
    /* The malloc failed hook is enabled by setting
    configUSE_MALLOC_FAILED_HOOK to 1 in FreeRTOSConfig.h.

    Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
    TT_LOG_E(TAG, "Malloc failed");
    for (;;)
        ;
}

void vApplicationStackOverflowHook(TT_UNUSED TaskHandle_t xTask, TT_UNUSED char* pcTaskName) {
    /* Run time stack overflow checking is performed if
    configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected.  pxCurrentTCB can be
    inspected in the debugger if the task name passed into this function is
    corrupt. */
    TT_LOG_E(TAG, "Task overflow in %s", pcTaskName);
    tt_crash_implementation();
}

void vApplicationIdleHook() {
    volatile size_t xFreeStackSpace;

    /* The idle task hook is enabled by setting configUSE_IDLE_HOOK to 1 in
    FreeRTOSConfig.h.

    This function is called on each cycle of the idle task.  In this case it
    does nothing useful, other than report the amount of FreeRTOS heap that
    remains unallocated. */
    //    xFreeStackSpace = xPortGetFreeHeapSize();

    //    if (xFreeStackSpace > 100) {
    /* By now, the kernel has allocated everything it is going to, so
        if there is a lot of heap remaining unallocated then
        the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
        reduced accordingly. */
    //    }
}

/**
 * Assert implementation as defined in the FreeRTOSConfig.h
 * It allows you to set breakpoints and debug asserts.
 */
void vAssertCalled(TT_UNUSED unsigned long line, TT_UNUSED const char* const file) {
    static portBASE_TYPE xPrinted = pdFALSE;
    volatile uint32_t set_to_nonzero_in_debugger_to_continue = 0;

    taskENTER_CRITICAL();
    {
        // Step out by attaching a debugger and setting set_to_nonzero_in_debugger_to_continue
        while (set_to_nonzero_in_debugger_to_continue == 0) { /* NO-OP */
        }
    }
    taskEXIT_CRITICAL();
}
