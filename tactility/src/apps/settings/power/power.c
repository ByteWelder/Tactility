#include "app.h"
#include "assets.h"
#include "lvgl.h"
#include "preferences.h"
#include "tactility.h"
#include "ui/toolbar.h"

#define TAG "power"

static void app_show(App app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    tt_toolbar_create_for_app(parent, app);

    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_style_border_width(wrapper, 0, 0);
    lv_obj_set_flex_grow(wrapper, 1);

    const Config* config = tt_get_config();
    if (config->hardware->power != NULL) {
        lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);

        // Build info
        lv_obj_t* power_charge_state = lv_label_create(wrapper);
        const char* charge_state = config->hardware->power->is_charging() ? "yes" : "no";
        lv_label_set_text_fmt(power_charge_state, "Charging: %s", charge_state);

        uint8_t charge_level = config->hardware->power->get_charge_level();
        uint16_t charge_level_scaled = (int16_t)charge_level * 100 / 255;
        lv_obj_t* power_charge_level = lv_label_create(wrapper);
        lv_label_set_text_fmt(power_charge_level, "Charge level: %d%%", charge_level_scaled);

        int32_t current = config->hardware->power->get_current();
        lv_obj_t* power_current = lv_label_create(wrapper);
        lv_label_set_text_fmt(power_current, "Current: %d mAh", current);
    } else {
        lv_obj_t* power_current = lv_label_create(wrapper);
        lv_label_set_text_fmt(power_current, "Not supported or implemented.");
        lv_obj_align(power_current, LV_ALIGN_CENTER, 0, 0);
    }
}

static void app_hide(TT_UNUSED App app) {
}

const AppManifest power_app = {
    .id = "power",
    .name = "Power",
    .icon = TT_ASSETS_APP_ICON_POWER_SETTINGS,
    .type = AppTypeSettings,
    .on_start = NULL,
    .on_stop = NULL,
    .on_show = &app_show,
    .on_hide = &app_hide
};
