#include <Tactility/TactilityCore.h>
#include <Tactility/TactilityPrivate.h>
#include <Tactility/app/AppContext.h>
#include <Tactility/app/AppPaths.h>
#include <Tactility/CpuAffinity.h>
#include <Tactility/hal/display/DisplayDevice.h>
#include <Tactility/hal/usb/Usb.h>
#include <Tactility/kernel/SystemEvents.h>
#include <Tactility/lvgl/Style.h>
#include <Tactility/service/loader/Loader.h>
#include <Tactility/settings/BootSettings.h>
#include <Tactility/settings/DisplaySettings.h>

#include <lvgl.h>

#ifdef ESP_PLATFORM
#include "Tactility/app/crashdiagnostics/CrashDiagnostics.h"
#include <Tactility/kernel/PanicHandler.h>
#include <sdkconfig.h>
#else
#define CONFIG_TT_SPLASH_DURATION 0
#endif

namespace tt::app::boot {

constexpr auto* TAG = "Boot";
extern const AppManifest manifest;

static std::shared_ptr<hal::display::DisplayDevice> getHalDisplay() {
    return hal::findFirstDevice<hal::display::DisplayDevice>(hal::Device::Type::Display);
}

class BootApp : public App {

    Thread thread = Thread(
        "boot",
        5120,
        [] { return bootThreadCallback(); },
        getCpuAffinityConfiguration().system
    );

    static void setupDisplay() {
        const auto hal_display = getHalDisplay();
        if (hal_display == nullptr) {
            return;
        }

        settings::display::DisplaySettings settings;
        if (settings::display::load(settings)) {
            if (hal_display->getGammaCurveCount() > 0) {
                hal_display->setGammaCurve(settings.gammaCurve);
                TT_LOG_I(TAG, "Gamma curve %du", settings.gammaCurve);
            }
        } else {
            settings = settings::display::getDefault();
        }

        if (hal_display->supportsBacklightDuty()) {
            TT_LOG_I(TAG, "Backlight %du", settings.backlightDuty);
            hal_display->setBacklightDuty(settings.backlightDuty);
        } else {
            TT_LOG_I(TAG, "no backlight");
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
        TT_LOG_I(TAG, "Starting boot thread");
        const auto start_time = kernel::getTicks();

        // Give the UI some time to redraw
        // If we don't do this, various init calls will read files and block SPI IO for the display
        // This would result in a blank/black screen being shown during this phase of the boot process
        // This works with 5 ms on a T-Lora Pager, so we give it 10 ms to be safe
        TT_LOG_I(TAG, "Delay");
        kernel::delayMillis(10);

        // TODO: Support for multiple displays
        TT_LOG_I(TAG, "Setup display");
        setupDisplay(); // Set backlight

        // This event will likely block as other systems are initialized
        // e.g. Wi-Fi reads AP configs from SD card
        TT_LOG_I(TAG, "Publish event");
        kernel::publishSystemEvent(kernel::SystemEvent::BootSplash);

        if (!setupUsbBootMode()) {
            TT_LOG_I(TAG, "initFromBootApp");
            initFromBootApp();
            waitForMinimalSplashDuration(start_time);
            stop(manifest.appId);
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

        settings::BootSettings boot_properties;
        if (!settings::loadBootSettings(boot_properties) || boot_properties.launcherAppId.empty()) {
            TT_LOG_E(TAG, "Launcher not configured");
            return;
        }

        start(boot_properties.launcherAppId);
    }

    static int getSmallestDimension() {
        auto* display = lv_display_get_default();
        int width = lv_display_get_horizontal_resolution(display);
        int height = lv_display_get_vertical_resolution(display);
        return std::min(width, height);
    }

public:

    void onCreate(AppContext& app) override {
        // Just in case this app is somehow resumed
        if (thread.getState() == Thread::State::Stopped) {
            thread.start();
        }
    }

    void onDestroy(AppContext& app) override {
        thread.join();
    }

    void onShow(TT_UNUSED AppContext& app, lv_obj_t* parent) override {
        lvgl::obj_set_style_bg_blacken(parent);
        lv_obj_set_style_border_width(parent, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_radius(parent, 0, LV_STATE_DEFAULT);

        auto* image = lv_image_create(parent);
        lv_obj_set_size(image, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_align(image, LV_ALIGN_CENTER, 0, 0);

        const auto paths = app.getPaths();
        const char* logo;
        // TODO: Replace with automatic asset buckets like on Android
        if (getSmallestDimension() < 150) { // e.g. Cardputer
            logo = hal::usb::isUsbBootMode() ? "logo_usb.png" : "logo_small.png";
        } else {
            logo = hal::usb::isUsbBootMode() ? "logo_usb.png" : "logo.png";
        }
        const auto logo_path = "A:" + paths->getAssetsPath(logo);
        TT_LOG_I(TAG, "%s", logo_path.c_str());
        lv_image_set_src(image, logo_path.c_str());
    }
};

extern const AppManifest manifest = {
    .appId = "Boot",
    .appName = "Boot",
    .appCategory = Category::System,
    .appFlags = AppManifest::Flags::HideStatusBar | AppManifest::Flags::Hidden,
    .createApp = create<BootApp>
};

} // namespace
