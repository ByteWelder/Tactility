#include <Tactility/CoreDefines.h>
#include <Tactility/Timer.h>
#include <Tactility/hal/display/DisplayDevice.h>
#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/service/ServiceContext.h>
#include <Tactility/service/ServiceManifest.h>
#include <Tactility/service/ServiceRegistration.h>
#include <Tactility/settings/DisplaySettings.h>

namespace tt::service::displayidle {

constexpr auto* TAG = "DisplayIdle";

class DisplayIdleService final : public Service {

    std::unique_ptr<Timer> timer;
    bool displayDimmed = false;
    settings::display::DisplaySettings cachedDisplaySettings;

    static std::shared_ptr<hal::display::DisplayDevice> getDisplay() {
        return hal::findFirstDevice<hal::display::DisplayDevice>(hal::Device::Type::Display);
    }

    void tick() {
        // Settings are now cached and event-driven (no file I/O in timer callback!)
        // This prevents watchdog timeout from blocking the Timer Service task

        // Query LVGL inactivity once for both checks
        uint32_t inactive_ms = 0;
        if (lvgl::lock(100)) {
            inactive_ms = lv_disp_get_inactive_time(nullptr);
            lvgl::unlock();
        }

        // Handle display backlight
        auto display = getDisplay();
        if (display != nullptr && display->supportsBacklightDuty()) {
            // If timeout disabled, ensure backlight restored if we had dimmed it
            if (!cachedDisplaySettings.backlightTimeoutEnabled || cachedDisplaySettings.backlightTimeoutMs == 0) {
                if (displayDimmed) {
                    display->setBacklightDuty(cachedDisplaySettings.backlightDuty);
                    displayDimmed = false;
                }
            } else {
                if (!displayDimmed && inactive_ms >= cachedDisplaySettings.backlightTimeoutMs) {
                    display->setBacklightDuty(0);
                    displayDimmed = true;
                } else if (displayDimmed && inactive_ms < 100) {
                    display->setBacklightDuty(cachedDisplaySettings.backlightDuty);
                    displayDimmed = false;
                }
            }
        }
    }

public:
    bool onStart(TT_UNUSED ServiceContext& service) override {
        // Load settings once at startup and cache them
        // This eliminates file I/O from timer callback (prevents watchdog timeout)
        cachedDisplaySettings = settings::display::loadOrGetDefault();
        
        // Note: Settings changes require service restart to take effect
        // TODO: Add DisplaySettingsChanged events for dynamic updates
        
        timer = std::make_unique<Timer>(Timer::Type::Periodic, kernel::millisToTicks(250), [this]{ this->tick(); });
        timer->setCallbackPriority(Thread::Priority::Lower);
        timer->start();
        return true;
    }

    void onStop(TT_UNUSED ServiceContext& service) override {
        if (timer) {
            timer->stop();
            timer = nullptr;
        }
        // Ensure display restored on stop
        auto display = getDisplay();
        if (display && displayDimmed) {
            display->setBacklightDuty(cachedDisplaySettings.backlightDuty);
            displayDimmed = false;
        }
    }
};

extern const ServiceManifest manifest = {
    .id = "DisplayIdle",
    .createService = create<DisplayIdleService>
};

}
