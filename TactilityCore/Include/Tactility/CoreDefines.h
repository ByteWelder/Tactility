#pragma once

#include "CoreExtraDefines.h"

#ifdef ESP_PLATFORM
#else
#include "portmacro.h"
#endif

#define TT_RETURNS_NONNULL __attribute__((returns_nonnull))

#define TT_WARN_UNUSED __attribute__((warn_unused_result))

#define TT_UNUSED __attribute__((unused))

#define TT_WEAK __attribute__((weak))

#define TT_PACKED __attribute__((packed))

#define TT_PLACE_IN_SECTION(x) __attribute__((section(x)))

#define TT_ALIGN(n) __attribute__((aligned(n)))

// Used by portENABLE_INTERRUPTS and portDISABLE_INTERRUPTS?
#ifdef ESP_PLATFORM
#define TT_IS_IRQ_MODE() (xPortInIsrContext() == pdTRUE)
#else
#define TT_IS_IRQ_MODE() false
#endif

#define TT_IS_ISR() (TT_IS_IRQ_MODE())

#define TT_CHECK_RETURN __attribute__((__warn_unused_result__))

// region Variable arguments support

// Adapted from https://stackoverflow.com/a/78848701/3848666
#define TT_ARG_IGNORE(X)
#define TT_ARG_CAT(X,Y) _TT_ARG_CAT(X,Y)
#define _TT_ARG_CAT(X,Y) X ## Y
#define TT_ARGCOUNT(...)     _TT_ARGCOUNT ## __VA_OPT__(1(__VA_ARGS__) TT_ARG_IGNORE) (0)
#define _TT_ARGCOUNT1(X,...) _TT_ARGCOUNT ## __VA_OPT__(2(__VA_ARGS__) TT_ARG_IGNORE) (1)
#define _TT_ARGCOUNT2(X,...) _TT_ARGCOUNT ## __VA_OPT__(3(__VA_ARGS__) TT_ARG_IGNORE) (2)
#define _TT_ARGCOUNT3(X,...) _TT_ARGCOUNT ## __VA_OPT__(4(__VA_ARGS__) TT_ARG_IGNORE) (3)
#define _TT_ARGCOUNT4(X,...) _TT_ARGCOUNT ## __VA_OPT__(5(__VA_ARGS__) TT_ARG_IGNORE) (4)
#define _TT_ARGCOUNT5(X,...) 5
#define _TT_ARGCOUNT(X) X
