#include "Tactility/TactilityCore.h"

#include "Tactility/app/AppContext.h"
#include "Tactility/app/display/DisplaySettings.h"
#include "Tactility/service/loader/Loader.h"
#include "Tactility/lvgl/Style.h"

#include <Tactility/Tactility.h>
#include <Tactility/hal/Display.h>
#include <Tactility/hal/usb/Usb.h>
#include <Tactility/kernel/SystemEvents.h>

#include <lvgl.h>

#ifdef ESP_PLATFORM
#include "Tactility/app/crashdiagnostics/CrashDiagnostics.h"
#include <Tactility/kernel/PanicHandler.h>
#include <sdkconfig.h>
#else
#define CONFIG_TT_SPLASH_DURATION 0
#endif

#define TAG "boot"

namespace tt::app::boot {

static std::shared_ptr<tt::hal::Display> getHalDisplay() {
    return hal::findFirstDevice<hal::Display>(hal::Device::Type::Display);
}

class BootApp : public App {

private:

    Thread thread = Thread("boot", 4096, bootThreadCallback, this);

    static int32_t bootThreadCallback(TT_UNUSED void* context) {
        TickType_t start_time = kernel::getTicks();

        kernel::systemEventPublish(kernel::SystemEvent::BootSplash);

        auto hal_display = getHalDisplay();
        assert(hal_display != nullptr);
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
#ifdef ESP_PLATFORM
        esp_reset_reason_t reason = esp_reset_reason();
        if (reason == ESP_RST_PANIC) {
            app::crashdiagnostics::start();
            return;
        }
#endif

        auto* config = tt::getConfiguration();
        assert(!config->launcherAppId.empty());
        tt::service::loader::startApp(config->launcherAppId);
    }

public:

    void onShow(TT_UNUSED AppContext& app, lv_obj_t* parent) override {
        auto* image = lv_image_create(parent);
        lv_obj_set_size(image, LV_PCT(100), LV_PCT(100));

        auto paths = app.getPaths();
        const char* logo = hal::usb::isUsbBootMode() ? "logo_usb.png" : "logo.png";
        auto logo_path = paths->getSystemPathLvgl(logo);
        TT_LOG_I(TAG, "%s", logo_path.c_str());
        lv_image_set_src(image, logo_path.c_str());

        lvgl::obj_set_style_bg_blacken(parent);

        // Just in case this app is somehow resumed
        if (thread.getState() == Thread::State::Stopped) {
            thread.start();
        }
    }

    void onDestroy(AppContext& app) override {
        thread.join();
    }
};

extern const AppManifest manifest = {
    .id = "Boot",
    .name = "Boot",
    .type = Type::Boot,
    .createApp = create<BootApp>
};

} // namespace
