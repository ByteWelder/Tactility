#include "app.h"
#include "assets.h"
#include "lvgl.h"
#include "preferences.h"
#include "tactility.h"
#include "ui/spacer.h"
#include "ui/toolbar.h"

#define TAG "display"

static bool backlight_duty_set = false;
static uint8_t backlight_duty = 255;

static void slider_event_cb(lv_event_t* event) {
    lv_obj_t* slider = lv_event_get_target(event);
    const Config* config = tt_get_config();
    SetBacklightDuty set_backlight_duty = config->hardware->display.set_backlight_duty;

    if (set_backlight_duty != NULL) {
        int32_t slider_value = lv_slider_get_value(slider);

        backlight_duty = (uint8_t)slider_value;
        backlight_duty_set = true;

        set_backlight_duty(backlight_duty);
    }
}

#define ORIENTATION_LANDSCAPE 0
#define ORIENTATION_LANDSCAPE_FLIPPED 1
#define ORIENTATION_PORTRAIT_LEFT 2
#define ORIENTATION_PORTRAIT_RIGHT 3

static int orientation_setting_to_display_orientation(int setting) {
    if (setting == ORIENTATION_LANDSCAPE_FLIPPED) {
        return LV_DISPLAY_ROTATION_180;
    } else if (setting == ORIENTATION_PORTRAIT_LEFT) {
        return LV_DISPLAY_ROTATION_270;
    } else if (setting == ORIENTATION_PORTRAIT_RIGHT) {
        return LV_DISPLAY_ROTATION_90;
    } else {
        return LV_DISPLAY_ROTATION_0;
    }
}

static int display_orientation_to_orientation_setting(int orientation) {
    if (orientation == LV_DISPLAY_ROTATION_90) {
        return ORIENTATION_PORTRAIT_RIGHT;
    } else if (orientation == LV_DISPLAY_ROTATION_180) {
        return ORIENTATION_LANDSCAPE_FLIPPED;
    } else if (orientation == LV_DISPLAY_ROTATION_270) {
        return ORIENTATION_PORTRAIT_LEFT;
    } else {
        return ORIENTATION_LANDSCAPE;
    }
}

static void on_orientation_set(lv_event_t* event) {
    lv_obj_t* dropdown = lv_event_get_target(event);
    int selected = lv_dropdown_get_selected(dropdown);
    TT_LOG_I(TAG, "Selected %d", selected);
    int rotation = orientation_setting_to_display_orientation(selected);
    if (lv_display_get_rotation(lv_display_get_default()) != rotation) {
        lv_display_set_rotation(lv_display_get_default(), rotation);
        tt_preferences()->put_int32("display", "rotation", rotation);
    }
}

static void app_show(App app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    tt_toolbar_create_for_app(parent, app);

    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_style_border_width(wrapper, 0, 0);
    lv_obj_set_flex_grow(wrapper, 1);

    lv_obj_t* brightness_wrapper = lv_obj_create(wrapper);
    lv_obj_set_size(brightness_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_t* brightness_label = lv_label_create(brightness_wrapper);
    lv_label_set_text(brightness_label, "Brightness");

    lv_obj_t* brightness_slider = lv_slider_create(brightness_wrapper);
    lv_obj_set_width(brightness_slider, LV_PCT(100));
    lv_slider_set_range(brightness_slider, 0, 255);
    lv_obj_add_event_cb(brightness_slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_pos(brightness_slider, 0, 30);

    const Config* config = tt_get_config();
    SetBacklightDuty set_backlight_duty = config->hardware->display.set_backlight_duty;
    if (set_backlight_duty == NULL) {
        lv_slider_set_value(brightness_slider, 255, LV_ANIM_OFF);
        lv_obj_add_state(brightness_slider, LV_STATE_DISABLED);
    } else {
        int32_t value = 255;
        tt_preferences()->opt_int32("display", "backlight_duty", &value);
        lv_slider_set_value(brightness_slider, value, LV_ANIM_OFF);
    }

    lv_obj_t* orientation_wrapper = lv_obj_create(wrapper);
    lv_obj_set_size(orientation_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_t* orientation_label = lv_label_create(orientation_wrapper);
    lv_label_set_text(orientation_label, "Orientation");
    lv_obj_align(orientation_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t* orientation_dropdown = lv_dropdown_create(orientation_wrapper);
    lv_dropdown_set_options(orientation_dropdown, "Landscape\nLandscape (flipped)\nPortrait Left\nPortrait Right");
    lv_obj_align(orientation_dropdown, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(orientation_dropdown, on_orientation_set, LV_EVENT_VALUE_CHANGED, NULL);
    int orientation_selected = display_orientation_to_orientation_setting(
        lv_display_get_rotation(lv_display_get_default())
    );
    lv_dropdown_set_selected(orientation_dropdown, orientation_selected);
}

static void app_hide(TT_UNUSED App app) {
    if (backlight_duty_set) {
        tt_preferences()->put_int32("display", "backlight_duty", backlight_duty);
    }
}

const AppManifest display_app = {
    .id = "display",
    .name = "Display",
    .icon = TT_ASSETS_APP_ICON_DISPLAY_SETTINGS,
    .type = AppTypeSettings,
    .on_start = NULL,
    .on_stop = NULL,
    .on_show = &app_show,
    .on_hide = &app_hide
};
