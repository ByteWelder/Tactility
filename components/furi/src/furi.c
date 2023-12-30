#include "furi.h"
#include "app_manifest_registry.h"
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

static bool scheduler_was_running = false;

void furi_init() {
    furi_assert(!furi_kernel_is_irq());

    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        vTaskSuspendAll();
        scheduler_was_running = true;
    }

    furi_record_init();

    xTaskResumeAll();

#if defined(__ARM_ARCH_7A__) && (__ARM_ARCH_7A__ == 0U)
    /* Service Call interrupt might be configured before kernel start      */
    /* and when its priority is lower or equal to BASEPRI, svc instruction */
    /* causes a Hard Fault.                                                */
    NVIC_SetPriority(SVCall_IRQn, 0U);
#endif

    app_manifest_registry_init();
}
