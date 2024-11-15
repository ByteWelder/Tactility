#include "preferences.h"
#include "display_preferences.h"

Preferences preferences("display");

#define BACKLIGHT_DUTY_KEY "backlight_duty"
#define ROTATION_KEY "rotation"

void display_preferences_set_backlight_duty(uint8_t value) {
    preferences.putInt32(BACKLIGHT_DUTY_KEY, (int32_t)value);
}

uint8_t display_preferences_get_backlight_duty() {
    int32_t result;
    if (preferences.optInt32(BACKLIGHT_DUTY_KEY, &result)) {
        return (uint8_t)(result % 255);
    } else {
        return 200;
    }
}

void display_preferences_set_rotation(lv_display_rotation_t rotation) {
    preferences.putInt32(ROTATION_KEY, (int32_t)rotation);
}

lv_display_rotation_t display_preferences_get_rotation() {
    int32_t rotation;
    if (preferences.optInt32(ROTATION_KEY, &rotation)) {
        return (lv_display_rotation_t)rotation;
    } else {
        return LV_DISPLAY_ROTATION_0;
    }
}
