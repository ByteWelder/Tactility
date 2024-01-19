#pragma once

#include "core_extra_defines.h"

#ifdef ESP_PLATFORM
#include "freertos/portmacro.h"
#else
#include "portmacro.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TT_RETURNS_NONNULL __attribute__((returns_nonnull))

#define TT_WARN_UNUSED __attribute__((warn_unused_result))

#define TT_WEAK __attribute__((weak))

#define TT_PACKED __attribute__((packed))

// Used by portENABLE_INTERRUPTS and portDISABLE_INTERRUPTS?
#ifdef ESP_TARGET
#define TT_IS_IRQ_MODE() (xPortInIsrContext() == pdTRUE)
#else
#define TT_IS_IRQ_MODE() false
#endif

#define TT_IS_ISR() (TT_IS_IRQ_MODE())

#define TT_CHECK_RETURN __attribute__((__warn_unused_result__))

#ifdef __cplusplus
}
#endif
