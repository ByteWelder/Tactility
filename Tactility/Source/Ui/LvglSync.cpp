#include "LvglSync.h"

namespace tt::lvgl {

static LvglLock lock_singleton = nullptr;
static LvglUnlock unlock_singleton = nullptr;

void sync_set(LvglLock lock, LvglUnlock unlock) {
    lock_singleton = lock;
    unlock_singleton = unlock;
}

bool lock(uint32_t timeout_ticks) {
    if (lock_singleton) {
        return lock_singleton(timeout_ticks);
    } else {
        return true;
    }
}

void unlock() {
    if (unlock_singleton) {
        unlock_singleton();
    }
}

} // namespace
