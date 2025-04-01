#include "Tactility/app/display/DisplaySettings.h"

#include <Tactility/Preferences.h>

namespace tt::app::display {

tt::Preferences preferences("display");

constexpr const char* BACKLIGHT_DUTY_KEY = "backlight_duty";
constexpr const char* GAMMA_CURVE_KEY = "gamma";
constexpr const char* ROTATION_KEY = "rotation";

void setBacklightDuty(uint8_t value) {
    preferences.putInt32(BACKLIGHT_DUTY_KEY, (int32_t)value);
}

bool getBacklightDuty(uint8_t& duty) {
    int32_t result;
    if (preferences.optInt32(BACKLIGHT_DUTY_KEY, result)) {
        duty = (uint8_t)(result % 256);
        return true;
    } else {
        return false;
    }
}

void setRotation(lv_display_rotation_t rotation) {
    preferences.putInt32(ROTATION_KEY, (int32_t)rotation);
}

lv_display_rotation_t getRotation() {
    int32_t rotation;
    if (preferences.optInt32(ROTATION_KEY, rotation)) {
        return (lv_display_rotation_t)rotation;
    } else {
        return LV_DISPLAY_ROTATION_0;
    }
}

void setGammaCurve(uint8_t curveIndex) {
    preferences.putInt32(GAMMA_CURVE_KEY, (int32_t)curveIndex);
}

bool getGammaCurve(uint8_t& curveIndex) {
    int32_t result;
    if (preferences.optInt32(GAMMA_CURVE_KEY, result)) {
        curveIndex = (uint8_t)(result % 256);
        return true;
    } else {
        return false;
    }
}

} // namespace
