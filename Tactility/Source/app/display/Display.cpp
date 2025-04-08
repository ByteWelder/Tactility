#include "Tactility/app/display/DisplaySettings.h"

#include "Tactility/hal/display/DisplayDevice.h"
#include "Tactility/lvgl/Toolbar.h"

#include <Tactility/Assets.h>
#include <Tactility/Tactility.h>

#include <lvgl.h>

namespace tt::app::display {

#define TAG "display"

static bool backlight_duty_set = false;
static uint8_t backlight_duty = 255;

static uint8_t gamma = 255;

#define ROTATION_DEFAULT 0
#define ROTATION_180 1
#define ROTATION_270 2
#define ROTATION_90 3

static std::shared_ptr<hal::display::DisplayDevice> getHalDisplay() {
    return hal::findFirstDevice<hal::display::DisplayDevice>(hal::Device::Type::Display);
}

static lv_display_rotation_t orientationSettingToDisplayRotation(uint32_t setting) {
    switch (setting) {
        case ROTATION_180:
            return LV_DISPLAY_ROTATION_180;
        case ROTATION_270:
            return LV_DISPLAY_ROTATION_270;
        case ROTATION_90:
            return LV_DISPLAY_ROTATION_90;
        default:
            return LV_DISPLAY_ROTATION_0;
    }
}

static uint32_t displayOrientationToOrientationSetting(lv_display_rotation_t orientation) {
    switch (orientation) {
        case LV_DISPLAY_ROTATION_90:
            return ROTATION_90;
        case LV_DISPLAY_ROTATION_180:
            return ROTATION_180;
        case LV_DISPLAY_ROTATION_270:
            return ROTATION_270;
        default:
            return ROTATION_DEFAULT;
    }
}

class DisplayApp : public App {

    static void onBacklightSliderEvent(lv_event_t* event) {
        auto* slider = static_cast<lv_obj_t*>(lv_event_get_target(event));

        auto hal_display = getHalDisplay();
        assert(hal_display != nullptr);

        if (hal_display->supportsBacklightDuty()) {
            int32_t slider_value = lv_slider_get_value(slider);

            backlight_duty = static_cast<uint8_t>(slider_value);
            backlight_duty_set = true;

            hal_display->setBacklightDuty(backlight_duty);
        }
    }

    static void onGammaSliderEvent(lv_event_t* event) {
        auto* slider = static_cast<lv_obj_t*>(lv_event_get_target(event));
        auto* lvgl_display = lv_display_get_default();
        assert(lvgl_display != nullptr);
        auto* hal_display = static_cast<hal::display::DisplayDevice*>(lv_display_get_user_data(lvgl_display));
        assert(hal_display != nullptr);

        if (hal_display->getGammaCurveCount() > 0) {
            int32_t slider_value = lv_slider_get_value(slider);

            gamma = static_cast<uint8_t>(slider_value);

            hal_display->setGammaCurve(gamma);
            setGammaCurve(gamma);
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

public:

    void onShow(AppContext& app, lv_obj_t* parent) override {
        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

        auto hal_display = getHalDisplay();
        assert(hal_display != nullptr);

        lvgl::toolbar_create(parent, app);

        auto* main_wrapper = lv_obj_create(parent);
        lv_obj_set_flex_flow(main_wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_width(main_wrapper, LV_PCT(100));
        lv_obj_set_flex_grow(main_wrapper, 1);

        if (hal_display->supportsBacklightDuty()) {
            auto* brightness_wrapper = lv_obj_create(main_wrapper);
            lv_obj_set_size(brightness_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_style_pad_hor(brightness_wrapper, 0, 0);
            lv_obj_set_style_pad_ver(brightness_wrapper, 6, 0);
            lv_obj_set_style_border_width(brightness_wrapper, 0, 0);

            auto* brightness_label = lv_label_create(brightness_wrapper);
            lv_label_set_text(brightness_label, "Brightness");

            auto* brightness_slider = lv_slider_create(brightness_wrapper);
            lv_obj_set_width(brightness_slider, LV_PCT(50));
            lv_obj_align(brightness_slider, LV_ALIGN_TOP_RIGHT, -8, 0);
            lv_slider_set_range(brightness_slider, 0, 255);
            lv_obj_add_event_cb(brightness_slider, onBacklightSliderEvent, LV_EVENT_VALUE_CHANGED, nullptr);

            uint8_t value;
            if (getBacklightDuty(value)) {
                lv_slider_set_value(brightness_slider, value, LV_ANIM_OFF);
            } else {
                lv_slider_set_value(brightness_slider, 0, LV_ANIM_OFF);
            }
        }

        if (hal_display->getGammaCurveCount() > 0) {
            auto* gamma_wrapper = lv_obj_create(main_wrapper);
            lv_obj_set_size(gamma_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_style_pad_hor(gamma_wrapper, 0, 0);
            lv_obj_set_style_pad_ver(gamma_wrapper, 6, 0);
            lv_obj_set_style_border_width(gamma_wrapper, 0, 0);

            auto* gamma_label = lv_label_create(gamma_wrapper);
            lv_label_set_text(gamma_label, "Gamma");
            lv_obj_set_y(gamma_label, 0);

            auto* gamma_slider = lv_slider_create(gamma_wrapper);
            lv_obj_set_width(gamma_slider, LV_PCT(50));
            lv_obj_align(gamma_slider, LV_ALIGN_TOP_RIGHT, -8, 0);
            lv_slider_set_range(gamma_slider, 0, hal_display->getGammaCurveCount());
            lv_obj_add_event_cb(gamma_slider, onGammaSliderEvent, LV_EVENT_VALUE_CHANGED, nullptr);

            uint8_t curve_index;
            if (getGammaCurve(curve_index)) {
                lv_slider_set_value(gamma_slider, curve_index, LV_ANIM_OFF);
            } else {
                lv_slider_set_value(gamma_slider, 0, LV_ANIM_OFF);
            }
        }

        auto* orientation_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_size(orientation_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(orientation_wrapper, 0, 0);
        lv_obj_set_style_border_width(orientation_wrapper, 0, 0);

        auto* orientation_label = lv_label_create(orientation_wrapper);
        lv_label_set_text(orientation_label, "Orientation");
        lv_obj_align(orientation_label, LV_ALIGN_TOP_LEFT, 0, 8);

        auto lvgl_display = lv_obj_get_display(parent);
        auto horizontal_px = lv_display_get_horizontal_resolution(lvgl_display);
        auto vertical_px = lv_display_get_vertical_resolution(lvgl_display);
        bool is_landscape_display = horizontal_px > vertical_px;

        auto* orientation_dropdown = lv_dropdown_create(orientation_wrapper);
        if (is_landscape_display) {
            lv_dropdown_set_options(orientation_dropdown, "Landscape\nLandscape (flipped)\nPortrait Left\nPortrait Right");
        } else {
            lv_dropdown_set_options(orientation_dropdown, "Portrait\nPortrait (flipped)\nLandscape Left\nLandscape Right");
        }

        lv_obj_align(orientation_dropdown, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_obj_add_event_cb(orientation_dropdown, onOrientationSet, LV_EVENT_VALUE_CHANGED, nullptr);
        uint32_t orientation_selected = displayOrientationToOrientationSetting(
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
