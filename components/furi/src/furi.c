#include "furi.h"

#include "app_manifest_registry.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "service_registry.h"

#define TAG "furi"

void furi_init() {
    FURI_LOG_I(TAG, "init start");
    furi_assert(!furi_kernel_is_irq());

#if defined(__ARM_ARCH_7A__) && (__ARM_ARCH_7A__ == 0U)
    /* Service Call interrupt might be configured before kernel start      */
    /* and when its priority is lower or equal to BASEPRI, svc instruction */
    /* causes a Hard Fault.                                                */
    NVIC_SetPriority(SVCall_IRQn, 0U);
#endif

    service_registry_init();
    app_manifest_registry_init();
    FURI_LOG_I(TAG, "init complete");
}
