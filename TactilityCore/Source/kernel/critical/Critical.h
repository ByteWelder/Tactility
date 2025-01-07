#pragma once

#include <cstdint>

namespace tt::kernel::critical {

typedef struct {
    uint32_t isrm;
    bool fromIsr;
    bool kernelRunning;
} TtCriticalInfo;

/** Enter a critical section
 * @return info on the status
 */
TtCriticalInfo enter();

/**
 * Exit a critical section
 * @param[in] info the info from when the critical section was started
 */
void exit(TtCriticalInfo info);

} // namespace
