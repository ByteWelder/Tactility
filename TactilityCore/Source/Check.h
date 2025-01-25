/**
 * @file check.h
 * 
 * Tactility crash and check functions.
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

#include "Log.h"
#include <cassert>
#include "CoreDefines.h"

#define TT_NORETURN [[noreturn]]

/** Crash system */
namespace tt {
    /**
     * Don't call this directly. Use tt_crash() as it will trace info.
     */
    TT_NORETURN void _crash();
}

/** Crash system with message. */
#define tt_crash(...) TT_ARG_CAT(_tt_crash,TT_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__)

#define _tt_crash0()  do {                                                  \
        TT_LOG_E("crash", "at %s:%d", __FILE__, __LINE__);                  \
        tt::_crash();                                                       \
    } while (0)

#define _tt_crash1(message) do {                                            \
        TT_LOG_E("crash", "%s\n\tat %s:%d", message, __FILE__, __LINE__);   \
        tt::_crash();                                                       \
    } while (0)

/** Halt the system
 * @param[in] optional message (const char*)
 */
#define tt_halt(...) M_APPLY(__tt_halt, M_IF_EMPTY(__VA_ARGS__)((NULL), (__VA_ARGS__)))

/** Check condition and crash if check failed */
#define tt_check_internal(__e, __m)        \
    do {                                   \
        if (!(__e)) {                      \
            TT_LOG_E("check", "%s", #__e); \
            if (__m) {                     \
                tt_crash_internal(#__m);   \
            } else {                       \
                tt_crash_internal("");     \
            }                              \
        }                                  \
    } while (0)

/** Check condition and crash if failed
 *
 * @param[in] condition to check
 * @param[in] optional message (const char*)
 */

#define tt_check(x, ...) if (!(x)) { TT_LOG_E("check", "Failed: %s", #x); tt::_crash(); }
