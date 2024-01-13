#pragma once

#include <stdio.h>
#include <stdbool.h>

#ifndef TT_CRITICAL_ENTER
#define TT_CRITICAL_ENTER() __TtCriticalInfo __tt_critical_info = __tt_critical_enter();
#endif

#ifndef TT_CRITICAL_EXIT
#define TT_CRITICAL_EXIT() __tt_critical_exit(__tt_critical_info);
#endif

typedef struct {
    uint32_t isrm;
    bool from_isr;
    bool kernel_running;
} __TtCriticalInfo;

__TtCriticalInfo __tt_critical_enter(void);

void __tt_critical_exit(__TtCriticalInfo info);
