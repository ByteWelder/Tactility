#include <tactility/module.h>
#include <tactility/delay.h>
#include <tactility/error.h>
#include <tactility/log.h>

#include <lvgl/lvgl.h>

#include <tactility/system_event.h>

#include <Tactility/LogMessages.h>
#include <Tactility/kernel/Kernel.h>
#include <Tactility/settings/TrackballSettings.h>

#include <lilygo/drivers/trackball.h>
#include <lilygo/drivers/tdeck_power_on.h>

constexpr auto* TAG = "tdeck";

extern "C" {

static void on_boot_completed(struct SystemEvent* /*event*/, void* /*context*/) {
    auto tbSettings = tt::settings::trackball::loadOrGetDefault();
    lvgl_lock();
    if (trackball::init() != nullptr) {
        trackball::setMode(tbSettings.trackballMode == tt::settings::trackball::TrackballMode::Pointer
            ? trackball::Mode::Pointer
            : trackball::Mode::Encoder);
        trackball::setEncoderSensitivity(tbSettings.encoderSensitivity);
        trackball::setPointerSensitivity(tbSettings.pointerSensitivity);
        trackball::setEnabled(tbSettings.trackballEnabled);
    }
    lvgl_unlock();
}

void subscribe_events() {
    // The kernel trackball device is already started by kernel_init(); this just registers it as an
    // LVGL input device and applies persisted settings, both of which require LVGL to be up first.
    system_event_subscribe(KERNEL_EVENT_BOOT_COMPLETED, on_boot_completed, nullptr);
}

static error_t start() {
    LOG_I(TAG, LOG_MESSAGE_POWER_ON_START);
    if (!tdeck_power_on()) {
        LOG_E(TAG, LOG_MESSAGE_POWER_ON_FAILED);
        return ERROR_RESOURCE;
    }

    // Avoids crash when no SD card is inserted. It's unknown why, but likely is related to power draw.
    delay_millis(100);

    subscribe_events();

    return ERROR_NONE;
}

static error_t stop() {
    return ERROR_NONE;
}

Module lilygo_tdeck_module = {
    .name = "lilygo-tdeck",
    .start = start,
    .stop = stop
};

}
