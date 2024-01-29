#include "app.h"
#include "lvgl.h"
#include "tactility.h"
#include "ui/spacer.h"
#include "ui/style.h"

static void slider_event_cb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    const Config* config = tt_get_config();
    SetBacklight set_backlight = config->hardware->display.set_backlight;
    if (set_backlight != NULL) {
        int32_t slider_value = lv_slider_get_value(slider);
        set_backlight((uint8_t)slider_value);
    }
}

static void app_show(TT_UNUSED App app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    tt_lv_obj_set_style_auto_padding(parent);

    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, "Brightness");

    tt_lv_spacer_create(parent, 1, 2);

    lv_obj_t* slider_container = lv_obj_create(parent);
    lv_obj_set_size(slider_container, LV_PCT(100), LV_SIZE_CONTENT);

    lv_obj_t* slider = lv_slider_create(slider_container);
    lv_obj_set_width(slider, LV_PCT(90));
    lv_obj_center(slider);
    lv_slider_set_range(slider, 0, 255);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    const Config* config = tt_get_config();
    SetBacklight set_backlight = config->hardware->display.set_backlight;
    if (set_backlight == NULL) {
        lv_slider_set_value(slider, 255, LV_ANIM_OFF);
        lv_obj_add_state(slider, LV_STATE_DISABLED);
    }
}

const AppManifest display_app = {
    .id = "display",
    .name = "Display",
    .icon = NULL,
    .type = AppTypeSettings,
    .on_start = NULL,
    .on_stop = NULL,
    .on_show = &app_show
};
