#include "core.h"

#include "app_manifest_registry.h"
#include "service_registry.h"

#define TAG "furi"

void tt_core_init() {
    TT_LOG_I(TAG, "init start");
    tt_assert(!tt_kernel_is_irq());

#if defined(__ARM_ARCH_7A__) && (__ARM_ARCH_7A__ == 0U)
    /* Service Call interrupt might be configured before kernel start      */
    /* and when its priority is lower or equal to BASEPRI, svc instruction */
    /* causes a Hard Fault.                                                */
    NVIC_SetPriority(SVCall_IRQn, 0U);
#endif

    tt_service_registry_init();
    tt_app_manifest_registry_init();
    TT_LOG_I(TAG, "init complete");
}
