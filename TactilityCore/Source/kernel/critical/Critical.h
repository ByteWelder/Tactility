#pragma once

#include <cstdint>

namespace tt::kernel::critical {

typedef struct {
    uint32_t isrm;
    bool fromIsr;
    bool kernelRunning;
} TtCriticalInfo;

TtCriticalInfo enter();

void exit(TtCriticalInfo info);

} // namespace
