#include "furi.h"
#include "app_manifest_registry.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void furi_init() {
    furi_assert(!furi_kernel_is_irq());

    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        vTaskSuspendAll();
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
