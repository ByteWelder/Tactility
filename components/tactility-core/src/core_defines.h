#pragma once

#include "core_extra_defines.h"
#include "freertos/portmacro.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <cmsis_compiler.h>

#define TT_RETURNS_NONNULL __attribute__((returns_nonnull))

#ifndef TT_WARN_UNUSED
#define TT_WARN_UNUSED __attribute__((warn_unused_result))
#endif

#ifndef TT_WEAK
#define TT_WEAK __attribute__((weak))
#endif

#ifndef TT_PACKED
#define TT_PACKED __attribute__((packed))
#endif

// Used by portENABLE_INTERRUPTS and portDISABLE_INTERRUPTS?
#ifndef TT_IS_IRQ_MODE
#define TT_IS_IRQ_MODE() (xPortInIsrContext() == pdTRUE)
#endif

#ifndef TT_IS_ISR
#define TT_IS_ISR() (TT_IS_IRQ_MODE())
#endif

#ifndef TT_CHECK_RETURN
#define TT_CHECK_RETURN __attribute__((__warn_unused_result__))
#endif

#ifdef __cplusplus
}
#endif
