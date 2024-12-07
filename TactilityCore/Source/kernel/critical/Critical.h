#pragma once

#include <cstdint>

#ifndef TT_CRITICAL_ENTER
#define TT_CRITICAL_ENTER() __TtCriticalInfo __tt_critical_info = __tt_critical_enter();
#endif

#ifndef TT_CRITICAL_EXIT
#define TT_CRITICAL_EXIT() __tt_critical_exit(__tt_critical_info);
#endif

namespace tt::kernel::critical {

typedef struct {
    uint32_t isrm;
    bool fromIsr;
    bool kernelRunning;
} TtCriticalInfo;

TtCriticalInfo enter();

void exit(TtCriticalInfo info);

} // namespace
