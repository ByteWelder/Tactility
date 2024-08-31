#include "lvgl_init_i.h"
#include "preferences.h"
#include "lvgl.h"

void tt_lvgl_init(const HardwareConfig* config) {
    SetBacklightDuty set_backlight_duty = config->display.set_backlight_duty;
    if (set_backlight_duty != NULL) {
        int32_t backlight_duty = 200;
        if (!tt_preferences()->opt_int32("display", "backlight_duty", &backlight_duty)) {
            tt_preferences()->put_int32("display", "backlight_duty", backlight_duty);
        }
        int32_t safe_backlight_duty = TT_MIN(backlight_duty, 255);
        set_backlight_duty((uint8_t)safe_backlight_duty);
    }

    int32_t rotation;
    if (tt_preferences()->opt_int32("display", "rotation", &rotation)) {
        if (rotation != LV_DISPLAY_ROTATION_0) {
            lv_disp_set_rotation(lv_disp_get_default(), rotation);
        }
    }
}
