#include "app.h"
#include "lvgl.h"
#include "preferences.h"
#include "tactility.h"
#include "ui/spacer.h"
#include "ui/toolbar.h"

static bool backlight_duty_set = false;
static uint8_t backlight_duty = 255;

static void slider_event_cb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    const Config* config = tt_get_config();
    SetBacklightDuty set_backlight_duty = config->hardware->display.set_backlight_duty;

    if (set_backlight_duty != NULL) {
        int32_t slider_value = lv_slider_get_value(slider);

        backlight_duty = (uint8_t)slider_value;
        backlight_duty_set = true;

        set_backlight_duty(backlight_duty);
    }
}

static void app_show(TT_UNUSED App app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    tt_toolbar_create_for_app(parent, app);

    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);

    lv_obj_t* label = lv_label_create(wrapper);
    lv_label_set_text(label, "Brightness");

    tt_lv_spacer_create(wrapper, 1, 2);

    lv_obj_t* slider = lv_slider_create(wrapper);
    lv_obj_set_width(slider, LV_PCT(100));
    lv_slider_set_range(slider, 0, 255);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    const Config* config = tt_get_config();
    SetBacklightDuty set_backlight_duty = config->hardware->display.set_backlight_duty;
    if (set_backlight_duty == NULL) {
        lv_slider_set_value(slider, 255, LV_ANIM_OFF);
        lv_obj_add_state(slider, LV_STATE_DISABLED);
    } else {
        int32_t value = 255;
        tt_preferences()->opt_int32("display", "backlight_duty", &value);
        lv_slider_set_value(slider, value, LV_ANIM_OFF);
    }
}

static void app_hide(App app) {
    if (backlight_duty_set) {
        tt_preferences()->put_int32("display", "backlight_duty", backlight_duty);
    }
}

const AppManifest display_app = {
    .id = "display",
    .name = "Display",
    .icon = NULL,
    .type = AppTypeSettings,
    .on_start = NULL,
    .on_stop = NULL,
    .on_show = &app_show,
    .on_hide = &app_hide
};
