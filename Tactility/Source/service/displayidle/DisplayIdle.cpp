#include <Tactility/service/ServiceRegistration.h>
#include <Tactility/service/ServiceManifest.h>
#include <Tactility/service/ServiceContext.h>
#include <Tactility/Timer.h>
#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/settings/DisplaySettings.h>
#include <Tactility/settings/KeyboardSettings.h>
#include <Tactility/hal/display/DisplayDevice.h>
#include <Tactility/hal/keyboard/KeyboardDevice.h>

// Forward declare driver functions
namespace driver::keyboardbacklight {
    bool setBrightness(uint8_t brightness);
}

namespace tt::service::displayidle {

constexpr auto* TAG = "DisplayIdle";

class DisplayIdleService final : public Service {

    std::unique_ptr<Timer> timer;
    bool displayDimmed = false;
    bool keyboardDimmed = false;
    settings::display::DisplaySettings cachedDisplaySettings;
    settings::keyboard::KeyboardSettings cachedKeyboardSettings;

    static std::shared_ptr<hal::display::DisplayDevice> getDisplay() {
        return hal::findFirstDevice<hal::display::DisplayDevice>(hal::Device::Type::Display);
    }

    static std::shared_ptr<hal::keyboard::KeyboardDevice> getKeyboard() {
        return hal::findFirstDevice<hal::keyboard::KeyboardDevice>(hal::Device::Type::Keyboard);
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

        // Handle keyboard backlight
        auto keyboard = getKeyboard();
        if (keyboard != nullptr && keyboard->isAttached()) {
            if (!cachedKeyboardSettings.backlightTimeoutEnabled || cachedKeyboardSettings.backlightTimeoutMs == 0) {
                if (keyboardDimmed) {
                    driver::keyboardbacklight::setBrightness(cachedKeyboardSettings.backlightEnabled ? cachedKeyboardSettings.backlightBrightness : 0);
                    keyboardDimmed = false;
                }
            } else {
                if (!keyboardDimmed && inactive_ms >= cachedKeyboardSettings.backlightTimeoutMs) {
                    driver::keyboardbacklight::setBrightness(0);
                    keyboardDimmed = true;
                } else if (keyboardDimmed && inactive_ms < 100) {
                    driver::keyboardbacklight::setBrightness(cachedKeyboardSettings.backlightEnabled ? cachedKeyboardSettings.backlightBrightness : 0);
                    keyboardDimmed = false;
                }
            }
        }
    }

public:
    bool onStart(TT_UNUSED ServiceContext& service) override {
        // Load settings once at startup and cache them
        // This eliminates file I/O from timer callback (prevents watchdog timeout)
        cachedDisplaySettings = settings::display::loadOrGetDefault();
        cachedKeyboardSettings = settings::keyboard::loadOrGetDefault();
        
        // Note: Settings changes require service restart to take effect
        // TODO: Add DisplaySettingsChanged/KeyboardSettingsChanged events for dynamic updates
        
        timer = std::make_unique<Timer>(Timer::Type::Periodic, [this]{ this->tick(); });
        timer->setThreadPriority(Thread::Priority::Lower);
        timer->start(250); // check 4x per second for snappy restore
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
        // Ensure keyboard backlight restored on stop
        auto keyboard = getKeyboard();
        if (keyboard && keyboardDimmed) {
            driver::keyboardbacklight::setBrightness(cachedKeyboardSettings.backlightEnabled ? cachedKeyboardSettings.backlightBrightness : 0);
            keyboardDimmed = false;
        }
    }
};

extern const ServiceManifest manifest = {
    .id = "DisplayIdle",
    .createService = create<DisplayIdleService>
};

}
