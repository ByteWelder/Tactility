#pragma once

#include "core_extra_defines.h"
#include "freertos/portmacro.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TT_RETURNS_NONNULL __attribute__((returns_nonnull))

#define TT_WARN_UNUSED __attribute__((warn_unused_result))

#define TT_WEAK __attribute__((weak))

#define TT_PACKED __attribute__((packed))

// Used by portENABLE_INTERRUPTS and portDISABLE_INTERRUPTS?
#define TT_IS_IRQ_MODE() (xPortInIsrContext() == pdTRUE)

#define TT_IS_ISR() (TT_IS_IRQ_MODE())

#define TT_CHECK_RETURN __attribute__((__warn_unused_result__))

#ifdef __cplusplus
}
#endif
