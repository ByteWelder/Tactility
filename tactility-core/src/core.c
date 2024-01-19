#include "core.h"

#include "app_manifest_registry.h"
#include "service_registry.h"

#define TAG "tactility"

void tt_core_init() {
    TT_LOG_I(TAG, "core init start");
    tt_assert(!tt_kernel_is_irq());
    tt_service_registry_init();
    tt_app_manifest_registry_init();
    TT_LOG_I(TAG, "core init complete");
}
