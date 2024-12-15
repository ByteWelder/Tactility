#include "Tactility.h"
#include "hal/sdcard/Sdcard.h"

static uint32_t fake_handle = 0;

static void* _Nullable sdcard_mount(TT_UNUSED const char* mount_point) {
    return &fake_handle;
}

static void* sdcard_init_and_mount(TT_UNUSED const char* mount_point) {
    return &fake_handle;
}

static void sdcard_unmount(TT_UNUSED void* context) {
}

static bool sdcard_is_mounted(TT_UNUSED void* context) {
    return TT_SCREENSHOT_MODE;
}

extern const tt::hal::sdcard::SdCard simulatorSdcard = {
    .mount = &sdcard_init_and_mount,
    .unmount = &sdcard_unmount,
    .is_mounted = &sdcard_is_mounted,
    .mount_behaviour = tt::hal::sdcard::MountBehaviourAtBoot
};
