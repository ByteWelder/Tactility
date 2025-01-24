#include "app/AppContext.h"
#include "Assets.h"
#include "DisplaySettings.h"
#include "Tactility.h"
#include "lvgl/Toolbar.h"
#include "lvgl.h"
#include "hal/Display.h"

namespace tt::app::display {

#define TAG "display"

static bool backlight_duty_set = false;
static uint8_t backlight_duty = 255;

static bool gamma_set = false;
static uint8_t gamma = 255;

#define ROTATION_DEFAULT 0
#define ROTATION_180 1
#define ROTATION_270 2
#define ROTATION_90 3

static void onBacklightSliderEvent(lv_event_t* event) {
    auto* slider = static_cast<lv_obj_t*>(lv_event_get_target(event));
    auto* lvgl_display = lv_display_get_default();
    assert(lvgl_display != nullptr);
    auto* hal_display = (tt::hal::Display*)lv_display_get_user_data(lvgl_display);
    assert(hal_display != nullptr);

    if (hal_display->supportsBacklightDuty()) {
        int32_t slider_value = lv_slider_get_value(slider);

        backlight_duty = (uint8_t)slider_value;
        backlight_duty_set = true;

        hal_display->setBacklightDuty(backlight_duty);
    }
}

static void onGammaSliderEvent(lv_event_t* event) {
    auto* slider = static_cast<lv_obj_t*>(lv_event_get_target(event));
    auto* lvgl_display = lv_display_get_default();
    assert(lvgl_display != nullptr);
    auto* hal_display = (tt::hal::Display*)lv_display_get_user_data(lvgl_display);
    assert(hal_display != nullptr);

    if (hal_display->getGammaCurveCount() > 0) {
        int32_t slider_value = lv_slider_get_value(slider);

        gamma = (uint8_t)slider_value;
        gamma_set = true;

        hal_display->setGammaCurve(gamma);
    }
}

static tt::hal::Display* getHalDisplay(lv_obj_t* widget) {
    auto* lvgl_display = lv_obj_get_display(widget);
    assert(lvgl_display != nullptr);
    auto* hal_display = (tt::hal::Display*)lv_display_get_user_data(lvgl_display);
    assert(hal_display != nullptr);
    return hal_display;
}

static lv_display_rotation_t orientationSettingToDisplayRotation(uint32_t setting) {
    if (setting == ROTATION_180) {
        return LV_DISPLAY_ROTATION_180;
    } else if (setting == ROTATION_270) {
        return LV_DISPLAY_ROTATION_270;
    } else if (setting == ROTATION_90) {
        return LV_DISPLAY_ROTATION_90;
    } else {
        return LV_DISPLAY_ROTATION_0;
    }
}

static uint32_t dipslayOrientationToOrientationSetting(lv_display_rotation_t orientation) {
    if (orientation == LV_DISPLAY_ROTATION_90) {
        return ROTATION_90;
    } else if (orientation == LV_DISPLAY_ROTATION_180) {
        return ROTATION_180;
    } else if (orientation == LV_DISPLAY_ROTATION_270) {
        return ROTATION_270;
    } else {
        return ROTATION_DEFAULT;
    }
}

static void onOrientationSet(lv_event_t* event) {
    auto* dropdown = static_cast<lv_obj_t*>(lv_event_get_target(event));
    uint32_t selected = lv_dropdown_get_selected(dropdown);
    TT_LOG_I(TAG, "Selected %ld", selected);
    lv_display_rotation_t rotation = orientationSettingToDisplayRotation(selected);
    if (lv_display_get_rotation(lv_display_get_default()) != rotation) {
        lv_display_set_rotation(lv_display_get_default(), rotation);
        setRotation(rotation);
    }
}

class DisplayApp : public App {

    void onShow(AppContext& app, lv_obj_t* parent) override {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

        lvgl::toolbar_create(parent, app);

        lv_obj_t* main_wrapper = lv_obj_create(parent);
        lv_obj_set_flex_flow(main_wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_width(main_wrapper, LV_PCT(100));
        lv_obj_set_flex_grow(main_wrapper, 1);

        lv_obj_t* wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_width(wrapper, LV_PCT(100));
        lv_obj_set_style_pad_all(wrapper, 8, 0);
        lv_obj_set_style_border_width(wrapper, 0, 0);

        lv_obj_t* brightness_label = lv_label_create(wrapper);
        lv_label_set_text(brightness_label, "Brightness");

        lv_obj_t* brightness_slider = lv_slider_create(wrapper);
        lv_obj_set_width(brightness_slider, LV_PCT(50));
        lv_obj_align(brightness_slider, LV_ALIGN_TOP_RIGHT, -8, 0);
        lv_slider_set_range(brightness_slider, 0, 255);
        lv_obj_add_event_cb(brightness_slider, onBacklightSliderEvent, LV_EVENT_VALUE_CHANGED, nullptr);

        lv_obj_t* gamma_label = lv_label_create(wrapper);
        lv_label_set_text(gamma_label, "Gamma");
        lv_obj_set_y(gamma_label, 40);

        lv_obj_t* gamma_slider = lv_slider_create(wrapper);
        lv_obj_set_width(gamma_slider, LV_PCT(50));
        lv_obj_align(gamma_slider, LV_ALIGN_TOP_RIGHT, -8, 40);
        lv_slider_set_range(gamma_slider, 0, getHalDisplay(parent)->getGammaCurveCount());
        lv_obj_add_event_cb(gamma_slider, onGammaSliderEvent, LV_EVENT_VALUE_CHANGED, nullptr);

        auto* hal_display = getHalDisplay(parent);
        assert(hal_display != nullptr);

        if (!hal_display->supportsBacklightDuty()) {
            lv_slider_set_value(brightness_slider, 255, LV_ANIM_OFF);
            lv_obj_add_state(brightness_slider, LV_STATE_DISABLED);
        } else {
            uint8_t value = getBacklightDuty();
            lv_slider_set_value(brightness_slider, value, LV_ANIM_OFF);
        }

        lv_slider_set_value(gamma_slider, 128, LV_ANIM_OFF);

        lv_obj_t* orientation_label = lv_label_create(wrapper);
        lv_label_set_text(orientation_label, "Orientation");
        lv_obj_align(orientation_label, LV_ALIGN_TOP_LEFT, 0, 80);

        auto lvgl_display = lv_obj_get_display(parent);
        auto horizontal_px = lv_display_get_horizontal_resolution(lvgl_display);
        auto vertical_px = lv_display_get_vertical_resolution(lvgl_display);
        bool is_landscape_display = horizontal_px > vertical_px;

        lv_obj_t* orientation_dropdown = lv_dropdown_create(wrapper);
        if (is_landscape_display) {
            lv_dropdown_set_options(orientation_dropdown, "Landscape\nLandscape (flipped)\nPortrait Left\nPortrait Right");
        } else {
            lv_dropdown_set_options(orientation_dropdown, "Portrait\nPortrait (flipped)\nLandscape Left\nLandscape Right");
        }

        lv_obj_align(orientation_dropdown, LV_ALIGN_TOP_RIGHT, 0, 72);
        lv_obj_add_event_cb(orientation_dropdown, onOrientationSet, LV_EVENT_VALUE_CHANGED, nullptr);
        uint32_t orientation_selected = dipslayOrientationToOrientationSetting(
            lv_display_get_rotation(lv_display_get_default())
        );
        lv_dropdown_set_selected(orientation_dropdown, orientation_selected);
    }

    void onHide(TT_UNUSED AppContext& app) override {
        if (backlight_duty_set) {
            setBacklightDuty(backlight_duty);
        }
    }
};

extern const AppManifest manifest = {
    .id = "Display",
    .name = "Display",
    .icon = TT_ASSETS_APP_ICON_DISPLAY_SETTINGS,
    .type = Type::Settings,
    .createApp = create<DisplayApp>
};

} // namespace
