#include "Tactility/TactilityCore.h"

#include "Tactility/app/AppContext.h"
#include "Tactility/app/display/DisplaySettings.h"
#include "Tactility/service/loader/Loader.h"
#include "Tactility/lvgl/Style.h"

#include "Tactility/hal/display/DisplayDevice.h"
#include <Tactility/TactilityPrivate.h>
#include <Tactility/hal/usb/Usb.h>
#include <Tactility/kernel/SystemEvents.h>

#include <lvgl.h>
#include <Tactility/BootProperties.h>
#include <Tactility/CpuAffinity.h>

#ifdef ESP_PLATFORM
#include "Tactility/app/crashdiagnostics/CrashDiagnostics.h"
#include <Tactility/kernel/PanicHandler.h>
#include <sdkconfig.h>
#else
#define CONFIG_TT_SPLASH_DURATION 0
#endif

namespace tt::app::boot {

constexpr auto* TAG = "Boot";

static std::shared_ptr<hal::display::DisplayDevice> getHalDisplay() {
    return hal::findFirstDevice<hal::display::DisplayDevice>(hal::Device::Type::Display);
}

class BootApp : public App {

    Thread thread = Thread(
        "boot",
        4096,
        [] { return bootThreadCallback(); },
        getCpuAffinityConfiguration().system
    );

    static void setupDisplay() {
        const auto hal_display = getHalDisplay();
        assert(hal_display != nullptr);
        if (hal_display->supportsBacklightDuty()) {
            uint8_t backlight_duty = 200;
            display::getBacklightDuty(backlight_duty);
            TT_LOG_I(TAG, "backlight %du", backlight_duty);
            hal_display->setBacklightDuty(backlight_duty);
        } else {
            TT_LOG_I(TAG, "no backlight");
        }

        if (hal_display->getGammaCurveCount() > 0) {
            uint8_t gamma_curve;
            if (display::getGammaCurve(gamma_curve)) {
                hal_display->setGammaCurve(gamma_curve);
                TT_LOG_I(TAG, "gamma %du", gamma_curve);
            }
        }
    }


    static bool setupUsbBootMode() {
        if (!hal::usb::isUsbBootMode()) {
            return false;
        }

        TT_LOG_I(TAG, "Rebooting into mass storage device mode");
        hal::usb::resetUsbBootMode();
        hal::usb::startMassStorageWithSdmmc();

        return true;
    }

    static void waitForMinimalSplashDuration(TickType_t startTime) {
        const auto end_time = kernel::getTicks();
        const auto ticks_passed = end_time - startTime;
        constexpr auto minimum_ticks = (CONFIG_TT_SPLASH_DURATION / portTICK_PERIOD_MS);
        if (minimum_ticks > ticks_passed) {
            kernel::delayTicks(minimum_ticks - ticks_passed);
        }
    }

    static int32_t bootThreadCallback() {
        const auto start_time = kernel::getTicks();

        kernel::publishSystemEvent(kernel::SystemEvent::BootSplash);

        setupDisplay(); // Set backlight

        if (!setupUsbBootMode()) {
            initFromBootApp();
            waitForMinimalSplashDuration(start_time);
            service::loader::stopApp();
            startNextApp();
        }

        return 0;
    }

    static void startNextApp() {
#ifdef ESP_PLATFORM
        if (esp_reset_reason() == ESP_RST_PANIC) {
            crashdiagnostics::start();
            return;
        }
#endif

        BootProperties boot_properties;
        if (!loadBootProperties(boot_properties) || boot_properties.launcherAppId.empty()) {
            TT_LOG_E(TAG, "Launcher not configured");
            stop();
            return;
        }

        service::loader::startApp(boot_properties.launcherAppId);
    }

public:

    void onShow(TT_UNUSED AppContext& app, lv_obj_t* parent) override {
        auto* image = lv_image_create(parent);
        lv_obj_set_size(image, LV_PCT(100), LV_PCT(100));

        const auto paths = app.getPaths();
        const char* logo = hal::usb::isUsbBootMode() ? "logo_usb.png" : "logo.png";
        const auto logo_path = paths->getSystemPathLvgl(logo);
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
