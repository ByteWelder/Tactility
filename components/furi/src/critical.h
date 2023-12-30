#pragma once

#include <stdio.h>
#include <stdbool.h>

#ifndef FURI_CRITICAL_ENTER
#define FURI_CRITICAL_ENTER() __FuriCriticalInfo __furi_critical_info = __furi_critical_enter();
#endif

#ifndef FURI_CRITICAL_EXIT
#define FURI_CRITICAL_EXIT() __furi_critical_exit(__furi_critical_info);
#endif

typedef struct {
    uint32_t isrm;
    bool from_isr;
    bool kernel_running;
} __FuriCriticalInfo;

__FuriCriticalInfo __furi_critical_enter(void);

void __furi_critical_exit(__FuriCriticalInfo info);
