#pragma once

#include "Lockable.h"

#include <memory>

namespace tt::lvgl {

typedef bool (*LvglLock)(uint32_t timeout_ticks);
typedef void (*LvglUnlock)();

void syncSet(LvglLock lock, LvglUnlock unlock);
bool isSyncSet();
bool lock(uint32_t timeout_ticks);
void unlock();

std::shared_ptr<Lockable> getLvglSyncLockable();

} // namespace
