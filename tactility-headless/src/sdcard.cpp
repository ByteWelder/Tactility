#include "sdcard.h"

#include "Mutex.h"
#include "tactility_core.h"

#define TAG "sdcard"

static Mutex mutex(MutexTypeRecursive);

typedef struct {
    const SdCard* sdcard;
    void* context;
} MountData;

static MountData data = {
    .sdcard = nullptr,
    .context = nullptr
};

static bool sdcard_lock(uint32_t timeout_ticks) {
    return mutex.acquire(timeout_ticks) == TtStatusOk;
}

static void sdcard_unlock() {
    mutex.release();
}

bool tt_sdcard_mount(const SdCard* sdcard) {
    TT_LOG_I(TAG, "Mounting");

    if (data.sdcard != nullptr) {
        TT_LOG_E(TAG, "Failed to mount: already mounted");
        return false;
    }

    if (sdcard_lock(100)) {
        void* context = sdcard->mount(TT_SDCARD_MOUNT_POINT);
        data = (MountData) {
            .sdcard = sdcard,
            .context = context
        };
        sdcard_unlock();
        return (data.context != nullptr);
    } else {
        TT_LOG_E(TAG, "Failed to lock");
        return false;
    }
}

SdcardState tt_sdcard_get_state() {
    if (data.context == nullptr) {
        return SdcardStateUnmounted;
    } else if (data.sdcard->is_mounted(data.context)) {
        return SdcardStateMounted;
    } else {
        return SdcardStateError;
    }
}

bool tt_sdcard_unmount(uint32_t timeout_ticks) {
    TT_LOG_I(TAG, "Unmounting");
    bool result = false;

    if (sdcard_lock(timeout_ticks)) {
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
        sdcard_unlock();
    } else {
        TT_LOG_E(TAG, "Failed to lock in %lu ticks", timeout_ticks);
    }

    return result;
}
