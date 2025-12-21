#include "devices/Touch.h"
#include "DeviceLovyan.h"
#include "Tactility/Log.h"
#include <cassert>

namespace tt::hal::touch {

static const char *TAG = "LovyanTouch";

bool LovyanTouch::startLvgl(lv_display_t* display) {
    TT_LOG_I(TAG, "startLvgl");
    if (lovyan_get_indev() == nullptr) {
        TT_LOG_E(TAG, "Lovyan indev not created");
        return false;
    }
    return true;
}

bool LovyanTouch::stopLvgl() {
    // Lovyan binding owns the indev; nothing to do here
    return true;
}

lv_indev_t* _Nullable LovyanTouch::getLvglIndev() {
    return lovyan_get_indev();
}

std::shared_ptr<tt::hal::touch::TouchDriver> _Nullable LovyanTouch::getTouchDriver() {
    // Do not expose a driver while LVGL indev is active (avoids bus/indev conflicts)
    assert(lovyan_get_indev() == nullptr);

    if (!touchDriver) {
        class LovyanTouchDriver final : public tt::hal::touch::TouchDriver {
        public:
            bool getTouchedPoints(uint16_t* x, uint16_t* y, uint16_t* strength, uint8_t* pointCount, uint8_t maxPointCount) override {
                uint16_t tx = 0, ty = 0;
                if (getLovyan().getTouch(&tx, &ty)) {
                    if (maxPointCount > 0) {
                        x[0] = tx;
                        y[0] = ty;
                        if (strength) strength[0] = 0;
                        *pointCount = 1;
                        return true;
                    }
                }
                *pointCount = 0;
                return false;
            }
        };
        touchDriver = std::make_shared<LovyanTouchDriver>();
    }

    return touchDriver;
}

} // namespace tt::hal::touch
