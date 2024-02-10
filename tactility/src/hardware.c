#include "hardware_i.h"

#include "lvgl.h"
#include "preferences.h"
#include "sdcard_i.h"

#define TAG "hardware"

static void init_display_settings(const HardwareConfig* config) {
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
        if (rotation != LV_DISP_ROT_NONE) {
            lv_disp_set_rotation(lv_disp_get_default(), rotation);
        }
    }
}

void tt_hardware_init(const HardwareConfig* config) {
    if (config->bootstrap != NULL) {
        TT_LOG_I(TAG, "Bootstrapping");
        tt_check(config->bootstrap(), "bootstrap failed");
    }

    tt_sdcard_init();
    if (config->sdcard != NULL) {
        TT_LOG_I(TAG, "Mounting sdcard");
        tt_sdcard_mount(config->sdcard);
    }

    tt_check(config->init_lvgl, "lvlg init not set");
    tt_check(config->init_lvgl(), "lvgl init failed");

    init_display_settings(config);
}
