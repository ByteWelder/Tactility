#include "sdcard.h"

#include "mutex.h"
#include "tactility_core.h"
#include "ui/statusbar.h"

#define TAG "sdcard"

static Mutex* mutex = NULL;
static int8_t statusbar_icon = -1;

typedef struct {
    const SdCard* sdcard;
    void* context;
} MountData;

static MountData data = {
    .sdcard = NULL,
    .context = NULL
};

static void sdcard_ensure_initialized() {
    if (mutex == NULL) {
        mutex = tt_mutex_alloc(MutexTypeRecursive);
        statusbar_icon = tt_statusbar_icon_add("A:/assets/sdcard_unmounted.png");
    }
}

static bool sdcard_lock(uint32_t timeout_ticks) {
    sdcard_ensure_initialized();
    return tt_mutex_acquire(mutex, timeout_ticks) == TtStatusOk;
}

static void sdcard_unlock() {
    tt_mutex_release(mutex);
}

bool tt_sdcard_mount(const SdCard* sdcard) {
    TT_LOG_I(TAG, "Mounting");

    if (data.sdcard != NULL) {
        TT_LOG_E(TAG, "Failed to mount: already mounted");
        return false;
    }

    if (sdcard_lock(100)) {
        void* context = sdcard->mount(TT_SDCARD_MOUNT_POINT);
        data = (MountData) {
            .context = context,
            .sdcard = sdcard
        };
        sdcard_unlock();
        if (data.context != NULL) {
            tt_statusbar_icon_set_image(statusbar_icon, "A:/assets/sdcard_mounted.png");
            return true;
        } else {
            return false;
        }
    } else {
        TT_LOG_E(TAG, "Failed to lock");
        return false;
    }
}

bool tt_sdcard_is_mounted() {
    return data.context != NULL;
}

bool tt_sdcard_unmount(uint32_t timeout_ticks) {
    TT_LOG_I(TAG, "Unmounting");
    bool result = false;

    if (sdcard_lock(timeout_ticks)) {
        if (data.sdcard != NULL) {
            data.sdcard->unmount(data.context);
            data = (MountData) {
                .context = NULL,
                .sdcard = NULL
            };
            result = true;
            tt_statusbar_icon_set_image(statusbar_icon, "A:/assets/sdcard_unmounted.png");
        } else {
            TT_LOG_E(TAG, "Can't unmount: nothing mounted");
        }
        sdcard_unlock();
    } else {
        TT_LOG_E(TAG, "Failed to lock in %lu ticks", timeout_ticks);
    }

    return result;
}
