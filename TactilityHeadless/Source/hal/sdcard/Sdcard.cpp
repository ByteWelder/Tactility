#include "Sdcard.h"

#include "Mutex.h"
#include "TactilityCore.h"

namespace tt::hal::sdcard {

#define TAG "sdcard"

static Mutex mutex(Mutex::TypeRecursive);

typedef struct {
    const SdCard* sdcard;
    void* context;
} MountData;

static MountData data = {
    .sdcard = nullptr,
    .context = nullptr
};

static bool lock(uint32_t timeout_ticks) {
    return mutex.acquire(timeout_ticks) == TtStatusOk;
}

static void unlock() {
    mutex.release();
}

bool mount(const SdCard* sdcard) {
    TT_LOG_I(TAG, "Mounting");

    if (data.sdcard != nullptr) {
        TT_LOG_E(TAG, "Failed to mount: already mounted");
        return false;
    }

    if (lock(100)) {
        void* context = sdcard->mount(TT_SDCARD_MOUNT_POINT);
        data = (MountData) {
            .sdcard = sdcard,
            .context = context
        };
        unlock();
        return (data.context != nullptr);
    } else {
        TT_LOG_E(TAG, "Failed to lock");
        return false;
    }
}

State getState() {
    if (data.context == nullptr) {
        return StateUnmounted;
    } else if (data.sdcard->is_mounted(data.context)) {
        return StateMounted;
    } else {
        return StateError;
    }
}

bool unmount(uint32_t timeout_ticks) {
    TT_LOG_I(TAG, "Unmounting");
    bool result = false;

    if (lock(timeout_ticks)) {
        if (data.sdcard != nullptr) {
            data.sdcard->unmount(data.context);
            data = (MountData) {
                .sdcard = nullptr,
                .context = nullptr
            };
            result = true;
        } else {
            TT_LOG_E(TAG, "Can't unmount: nothing mounted");
        }
        unlock();
    } else {
        TT_LOG_E(TAG, "Failed to lock in %lu ticks", timeout_ticks);
    }

    return result;
}

} // namespace
