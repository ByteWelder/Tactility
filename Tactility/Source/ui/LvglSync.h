#pragma once

#include <cstdint>

namespace tt::lvgl {

typedef bool (*LvglLock)(uint32_t timeout_ticks);
typedef void (*LvglUnlock)();

void syncSet(LvglLock lock, LvglUnlock unlock);
bool lock(uint32_t timeout_ticks);
void unlock();

} // namespace
