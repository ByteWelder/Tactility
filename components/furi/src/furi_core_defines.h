#pragma once

#include "freertos/portmacro.h"
#include "furi_extra_defines.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <cmsis_compiler.h>

#ifndef FURI_WARN_UNUSED
#define FURI_WARN_UNUSED __attribute__((warn_unused_result))
#endif

#ifndef FURI_WEAK
#define FURI_WEAK __attribute__((weak))
#endif

#ifndef FURI_PACKED
#define FURI_PACKED __attribute__((packed))
#endif

// Used by portENABLE_INTERRUPTS and portDISABLE_INTERRUPTS?
#ifndef FURI_IS_IRQ_MODE
#define FURI_IS_IRQ_MODE() (xPortInIsrContext() == pdTRUE)
#endif

#ifndef FURI_IS_ISR
#define FURI_IS_ISR() (FURI_IS_IRQ_MODE())
#endif

#ifndef FURI_CHECK_RETURN
#define FURI_CHECK_RETURN __attribute__((__warn_unused_result__))
#endif

#ifdef __cplusplus
}
#endif
