#include <Mutex.h>
#include "LvglSync.h"

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
    lock_singleton = lock;
    unlock_singleton = unlock;
}

bool lock(TickType_t timeout) {
    return lock_singleton(pdMS_TO_TICKS(timeout == 0 ? portMAX_DELAY : timeout));
}

void unlock() {
    unlock_singleton();
}

class LvglSync : public Lockable {
public:
    ~LvglSync() override = default;

    bool lock(TickType_t timeoutTicks) const override {
        return tt::lvgl::lock(timeoutTicks);
    }

    bool unlock() const override {
        tt::lvgl::unlock();
        return true;
    }
};

static std::shared_ptr<Lockable> lvglSync = std::make_shared<LvglSync>();

std::shared_ptr<Lockable> getLvglSyncLockable() {
    return lvglSync;
}

} // namespace
