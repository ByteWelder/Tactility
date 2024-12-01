#include <Mutex.h>
#include "LvglSync.h"

namespace tt::lvgl {

Mutex lockMutex;

static bool defaultLock(uint32_t timeoutTicks) {
    return lockMutex.acquire(timeoutTicks) == TtStatusOk;
}

static void defaultUnlock() {
    lockMutex.release();
}

static LvglLock lock_singleton = defaultLock;
static LvglUnlock unlock_singleton = defaultUnlock;

void syncSet(LvglLock lock, LvglUnlock unlock) {
    lock_singleton = lock;
    unlock_singleton = unlock;
}

bool lock(uint32_t timeout_ticks) {
    return lock_singleton(timeout_ticks);
}

void unlock() {
    unlock_singleton();
}

} // namespace
