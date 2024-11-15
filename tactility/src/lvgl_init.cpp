#include "apps/settings/display/display_preferences.h"
#include "lvgl.h"
#include "lvgl_init_i.h"
#include "preferences.h"

void tt_lvgl_init(const HardwareConfig* config) {
    SetBacklightDuty set_backlight_duty = config->display.set_backlight_duty;
    if (set_backlight_duty != nullptr) {
        int32_t backlight_duty = display_preferences_get_backlight_duty();
        set_backlight_duty(backlight_duty);
    }

    lv_display_rotation_t rotation = display_preferences_get_rotation();
    if (rotation != lv_disp_get_rotation(lv_disp_get_default())) {
        lv_disp_set_rotation(lv_disp_get_default(), static_cast<lv_display_rotation_t>(rotation));
    }
}
