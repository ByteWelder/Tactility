#include "Tactility/lvgl/LvglSync.h"

#include <Tactility/Mutex.h>

namespace tt::lvgl {

static Mutex lockMutex;

static bool defaultLock(uint32_t timeoutMillis) {
    return lockMutex.lock(timeoutMillis);
}

static void defaultUnlock() {
    lockMutex.unlock();
}

static LvglLock lock_singleton = defaultLock;
static LvglUnlock unlock_singleton = defaultUnlock;

void syncSet(LvglLock lock, LvglUnlock unlock) {
    auto old_lock = lock_singleton;
    auto old_unlock = unlock_singleton;

    // Ensure the old lock is not engaged when changing locks
    old_lock((uint32_t)portMAX_DELAY);
    lock_singleton = lock;
    unlock_singleton = unlock;
    old_unlock();
}

bool lock(TickType_t timeout) {
    return lock_singleton(pdMS_TO_TICKS(timeout == 0 ? portMAX_DELAY : timeout));
}

void unlock() {
    unlock_singleton();
}

class LvglSync : public Lock {
public:
    ~LvglSync() override = default;

    bool lock(TickType_t timeoutTicks) const override {
        return lvgl::lock(timeoutTicks);
    }

    bool unlock() const override {
        lvgl::unlock();
        return true;
    }
};

static std::shared_ptr<Lock> lvglSync = std::make_shared<LvglSync>();

std::shared_ptr<Lock> getSyncLock() {
    return lvglSync;
}

} // namespace
