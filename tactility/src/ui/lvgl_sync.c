#include "lvgl_sync.h"

static LvglLock lock_singleton = NULL;
static LvglUnlock unlock_singleton = NULL;

void tt_lvgl_sync_set(LvglLock lock, LvglUnlock unlock) {
    lock_singleton = lock;
    unlock_singleton = unlock;
}

bool tt_lvgl_lock(int timeout_ticks) {
    if (lock_singleton) {
        return lock_singleton(timeout_ticks);
    } else {
        return true;
    }
}

void tt_lvgl_unlock() {
    if (unlock_singleton) {
        unlock_singleton();
    }
}
