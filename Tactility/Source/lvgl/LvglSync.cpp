#include "LvglSync.h"

#include "Check.h"

namespace tt::lvgl {

static LvglLock lock_singleton = nullptr;
static LvglUnlock unlock_singleton = nullptr;

void syncSet(LvglLock lock, LvglUnlock unlock) {
    lock_singleton = lock;
    unlock_singleton = unlock;
}

bool lock(uint32_t timeout_ticks) {
    tt_check(lock_singleton);
    return lock_singleton(timeout_ticks);
}

void unlock() {
    tt_check(unlock_singleton);
    unlock_singleton();
}

} // namespace
