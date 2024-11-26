#include "app/display/DisplayPreferences.h"
#include "lvgl.h"
#include "hal/Configuration.h"

namespace tt::lvgl {

void init(const hal::Configuration* config) {
    hal::SetBacklightDuty set_backlight_duty = config->display.setBacklightDuty;
    if (set_backlight_duty != nullptr) {
        int32_t backlight_duty = app::display::preferences_get_backlight_duty();
        set_backlight_duty(backlight_duty);
    }

    lv_display_rotation_t rotation = app::display::preferences_get_rotation();
    if (rotation != lv_disp_get_rotation(lv_disp_get_default())) {
        lv_disp_set_rotation(lv_disp_get_default(), static_cast<lv_display_rotation_t>(rotation));
    }
}

} // namespace
