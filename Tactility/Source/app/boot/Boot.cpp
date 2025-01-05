#include "Assets.h"
#include "TactilityCore.h"

#include "app/AppContext.h"
#include "app/display/DisplaySettings.h"
#include "hal/Display.h"
#include "service/loader/Loader.h"
#include "lvgl/Style.h"

#include "lvgl.h"
#include "Tactility.h"
#include "hal/usb/Usb.h"

#ifdef ESP_PLATFORM
#include "kernel/PanicHandler.h"
#include "sdkconfig.h"
#else
#define CONFIG_TT_SPLASH_DURATION 0
#endif

#define TAG "boot"

namespace tt::app::boot {

static int32_t bootThreadCallback(void* context);
static void startNextApp();

struct Data {
    Data() : thread("boot", 4096, bootThreadCallback, this) {}

    Thread thread;
};

static int32_t bootThreadCallback(TT_UNUSED void* context) {
    TickType_t start_time = kernel::getTicks();

    auto* lvgl_display = lv_display_get_default();
    tt_assert(lvgl_display != nullptr);
    auto* hal_display = (hal::Display*)lv_display_get_user_data(lvgl_display);
    tt_assert(hal_display != nullptr);
    if (hal_display->supportsBacklightDuty()) {
        int32_t backlight_duty = app::display::getBacklightDuty();
        hal_display->setBacklightDuty(backlight_duty);
    }

    if (hal::usb::isUsbBootMode()) {
        TT_LOG_I(TAG, "Rebooting into mass storage device mode");
        hal::usb::resetUsbBootMode();
        hal::usb::startMassStorageWithSdmmc();
    } else {
        TickType_t end_time = tt::kernel::getTicks();
        TickType_t ticks_passed = end_time - start_time;
        TickType_t minimum_ticks = (CONFIG_TT_SPLASH_DURATION / portTICK_PERIOD_MS);
        if (minimum_ticks > ticks_passed) {
            kernel::delayTicks(minimum_ticks - ticks_passed);
        }

        tt::service::loader::stopApp();
        startNextApp();
    }

    return 0;
}

static void startNextApp() {
    auto config = tt::getConfiguration();
    std::string next_app;
    if (config->autoStartAppId) {
        TT_LOG_I(TAG, "init auto-starting %s", config->autoStartAppId);
        next_app = config->autoStartAppId;
    } else {
        next_app = "Desktop";
    }

#ifdef ESP_PLATFORM
    esp_reset_reason_t reason = esp_reset_reason();
    if (reason == ESP_RST_PANIC) {
        tt::service::loader::startApp("CrashDiagnostics");
    } else {
        tt::service::loader::startApp(next_app);
    }
#else
    tt::service::loader::startApp(next_app);
#endif
}

static void onShow(TT_UNUSED AppContext& app, lv_obj_t* parent) {
    auto data = std::static_pointer_cast<Data>(app.getData());

    lv_obj_t* image = lv_image_create(parent);
    lv_obj_set_size(image, LV_PCT(100), LV_PCT(100));

    auto paths = app.getPaths();
    const char* logo = hal::usb::isUsbBootMode() ? "logo_usb.png" : "logo.png";
    auto logo_path = paths->getSystemPathLvgl(logo);
    TT_LOG_I(TAG, "%s", logo_path.c_str());
    lv_image_set_src(image, logo_path.c_str());

    lvgl::obj_set_style_bg_blacken(parent);

    data->thread.start();
}

static void onStart(AppContext& app) {
    auto data = std::make_shared<Data>();
    app.setData(data);
}

static void onStop(AppContext& app) {
    auto data = std::static_pointer_cast<Data>(app.getData());
    data->thread.join();
}

extern const AppManifest manifest = {
    .id = "Boot",
    .name = "Boot",
    .type = TypeBoot,
    .onStart = onStart,
    .onStop = onStop,
    .onShow = onShow,
};

} // namespace
