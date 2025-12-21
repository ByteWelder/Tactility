#include "devices/Display.h"
#include "devices/Touch.h"
#include "Tactility/Log.h"
#include "DeviceLovyan.h"

namespace tt::hal::display {

static const char *TAG = "LovyanDisplay";
static bool hw_initialized = false;

bool LovyanDisplay::start() {
    TT_LOG_I(TAG, "start");
    if (hw_initialized) {
        TT_LOG_W(TAG, "hardware already initialized");
        return true;
    }

    bool ok = lovyan_hw_init();
    if (ok) {
        hw_initialized = true;
    } else {
        TT_LOG_E(TAG, "lovyan_hw_init failed");
    }
    return ok;
}

bool LovyanDisplay::stop() {
    TT_LOG_I(TAG, "stop");
    stopLvgl();
    return true;
}

bool LovyanDisplay::startLvgl() {
    TT_LOG_I(TAG, "startLvgl");
    if (lovyan_get_display() != nullptr) {
        TT_LOG_W(TAG, "LVGL already started for this display");
        return false;
    }

    if (!lovyan_lvgl_bind()) {
        TT_LOG_E(TAG, "lovyan_lvgl_bind failed");
        return false;
    }

    // Create touch device wrapper if not exist
    if (!touchDevice) {
        touchDevice = std::make_shared<tt::hal::touch::LovyanTouch>();
    }

    return true;
}

bool LovyanDisplay::stopLvgl() {
    TT_LOG_I(TAG, "stopLvgl");
    auto td = touchDevice;
    if (td && td->getLvglIndev() != nullptr) {
        TT_LOG_I(TAG, "Stopping touch device");
        td->stopLvgl();
    }

    lovyan_lvgl_unbind();
    return true;
}

std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable LovyanDisplay::getTouchDevice() {
    if (!touchDevice) {
        touchDevice = std::make_shared<tt::hal::touch::LovyanTouch>();
    }
    return touchDevice;
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    return std::make_shared<LovyanDisplay>();
}

} // namespace tt::hal::display
