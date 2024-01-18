/**
 * @file check.h
 * 
 * Tactility crash and assert functions.
 * 
 * The main problem with crashing is that you can't do anything without disturbing registers,
 * and if you disturb registers, you won't be able to see the correct register values in the debugger.
 * 
 * Current solution works around it by passing the message through r12 and doing some magic with registers in crash function.
 * r0-r10 are stored in the ram2 on crash routine start and restored at the end.
 * The only register that is going to be lost is r11.
 * 
 */
#pragma once

#include "log.h"
#include <m-core.h>

#ifdef __cplusplus
extern "C" {
#define TT_NORETURN [[noreturn]]
#else
#include <stdnoreturn.h>
#define TT_NORETURN noreturn
#endif

/** Crash system */
TT_NORETURN void tt_crash_implementation();

/** Crash system with message. */
#define __tt_crash(message)                                                                    \
    do {                                                                                       \
        TT_LOG_E("crash", "%s\n\tat %s:%d", ((message) ? (message) : ""), __FILE__, __LINE__); \
        tt_crash_implementation();                                                             \
    } while (0)

/** Crash system
 *
 * @param      optional  message (const char*)
 */
#define tt_crash(...) M_APPLY(__tt_crash, M_IF_EMPTY(__VA_ARGS__)((NULL), (__VA_ARGS__)))

/** Halt system
 *
 * @param      optional  message (const char*)
 */
#define tt_halt(...) M_APPLY(__tt_halt, M_IF_EMPTY(__VA_ARGS__)((NULL), (__VA_ARGS__)))

/** Check condition and crash if check failed */
#define __tt_check(__e, __m)               \
    do {                                   \
        if (!(__e)) {                      \
            TT_LOG_E("check", "%s", #__e); \
            if (__m) {                     \
                __tt_crash(#__m);          \
            } else {                       \
                __tt_crash("");            \
            }                              \
        }                                  \
    } while (0)

/** Check condition and crash if failed
 *
 * @param      condition to check
 * @param      optional  message (const char*)
 */
#define tt_check(...) \
    M_APPLY(__tt_check, M_DEFAULT_ARGS(2, NULL, __VA_ARGS__))

/** Only in debug build: Assert condition and crash if assert failed  */
#ifdef TT_DEBUG
#define __tt_assert(__e, __m)               \
    do {                                    \
        if (!(__e)) {                       \
            TT_LOG_E("assert", "%s", #__e); \
            if (__m) {                      \
                __tt_crash(#__m);           \
            } else {                        \
                __tt_crash("");             \
            }                               \
        }                                   \
    } while (0)
#else
#define __tt_assert(__e, __m) \
    do {                      \
        ((void)(__e));        \
        ((void)(__m));        \
    } while (0)
#endif

/** Assert condition and crash if failed
 *
 * @warning    only will do check if firmware compiled in debug mode
 *
 * @param      condition to check
 * @param      optional  message (const char*)
 */
#define tt_assert(...) \
    M_APPLY(__tt_assert, M_DEFAULT_ARGS(2, NULL, __VA_ARGS__))

#ifdef __cplusplus
}
#endif
