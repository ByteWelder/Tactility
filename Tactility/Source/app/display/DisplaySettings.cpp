#include "app/display/DisplaySettings.h"

#include "Preferences.h"

namespace tt::app::display {

tt::Preferences preferences("display");

#define BACKLIGHT_DUTY_KEY "backlight_duty"
#define ROTATION_KEY "rotation"

void setBacklightDuty(uint8_t value) {
    preferences.putInt32(BACKLIGHT_DUTY_KEY, (int32_t)value);
}

uint8_t getBacklightDuty() {
    int32_t result;
    if (preferences.optInt32(BACKLIGHT_DUTY_KEY, result)) {
        return (uint8_t)(result % 256);
    } else {
        return 200;
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

} // namespace
