#include <Tactility/Tactility.h>

#include <Tactility/settings/DisplaySettings.h>
#include <Tactility/Assets.h>
#include <Tactility/hal/display/DisplayDevice.h>
#include <Tactility/lvgl/Toolbar.h>

#include <lvgl.h>

namespace tt::app::display {

constexpr auto* TAG = "Display";

static std::shared_ptr<hal::display::DisplayDevice> getHalDisplay() {
    return hal::findFirstDevice<hal::display::DisplayDevice>(hal::Device::Type::Display);
}

class DisplayApp final : public App {

    settings::display::DisplaySettings displaySettings;
    bool displaySettingsUpdated = false;

    static void onBacklightSliderEvent(lv_event_t* event) {
        auto* slider = static_cast<lv_obj_t*>(lv_event_get_target(event));
        auto* app = static_cast<DisplayApp*>(lv_event_get_user_data(event));
        auto hal_display = getHalDisplay();
        assert(hal_display != nullptr);

        if (hal_display->supportsBacklightDuty()) {
            int32_t slider_value = lv_slider_get_value(slider);
            app->displaySettings.backlightDuty = static_cast<uint8_t>(slider_value);
            app->displaySettingsUpdated = true;
            hal_display->setBacklightDuty(app->displaySettings.backlightDuty);
        }
    }

    static void onGammaSliderEvent(lv_event_t* event) {
        auto* slider = static_cast<lv_obj_t*>(lv_event_get_target(event));
        auto hal_display = hal::findFirstDevice<hal::display::DisplayDevice>(hal::Device::Type::Display);
        auto* app = static_cast<DisplayApp*>(lv_event_get_user_data(event));
        assert(hal_display != nullptr);

        if (hal_display->getGammaCurveCount() > 0) {
            int32_t slider_value = lv_slider_get_value(slider);
            app->displaySettings.gammaCurve = static_cast<uint8_t>(slider_value);
            app->displaySettingsUpdated = true;
            hal_display->setGammaCurve(app->displaySettings.gammaCurve);
        }
    }

    static void onOrientationSet(lv_event_t* event) {
        auto* app = static_cast<DisplayApp*>(lv_event_get_user_data(event));
        auto* dropdown = static_cast<lv_obj_t*>(lv_event_get_target(event));
        uint32_t selected_index = lv_dropdown_get_selected(dropdown);
        TT_LOG_I(TAG, "Selected %ld", selected_index);
        auto selected_orientation = static_cast<settings::display::Orientation>(selected_index);
        if (selected_orientation != app->displaySettings.orientation) {
            app->displaySettings.orientation = selected_orientation;
            app->displaySettingsUpdated = true;
            lv_display_set_rotation(lv_display_get_default(), settings::display::toLvglDisplayRotation(selected_orientation));
        }
    }

public:

    void onShow(AppContext& app, lv_obj_t* parent) override {
        displaySettings = settings::display::loadOrGetDefault();

        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

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
            lv_obj_add_event_cb(brightness_slider, onBacklightSliderEvent, LV_EVENT_VALUE_CHANGED, this);

            lv_slider_set_value(brightness_slider, displaySettings.backlightDuty, LV_ANIM_OFF);
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
            lv_obj_add_event_cb(gamma_slider, onGammaSliderEvent, LV_EVENT_VALUE_CHANGED, this);

            uint8_t curve_index = displaySettings.gammaCurve;
            lv_slider_set_value(gamma_slider, curve_index, LV_ANIM_OFF);
        }

        auto* orientation_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_size(orientation_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(orientation_wrapper, 0, 0);
        lv_obj_set_style_border_width(orientation_wrapper, 0, 0);

        auto* orientation_label = lv_label_create(orientation_wrapper);
        lv_label_set_text(orientation_label, "Orientation");
        lv_obj_align(orientation_label, LV_ALIGN_TOP_LEFT, 0, 8);

        auto* orientation_dropdown = lv_dropdown_create(orientation_wrapper);
        // Note: order correlates with settings::display::Orientation item order
        lv_dropdown_set_options(orientation_dropdown, "Landscape\nPortrait Right\nLandscape Flipped\nPortrait Left");
        lv_obj_align(orientation_dropdown, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_obj_set_style_border_color(orientation_dropdown, lv_color_hex(0xFAFAFA), LV_PART_MAIN);
        lv_obj_set_style_border_width(orientation_dropdown, 1, LV_PART_MAIN);
        lv_obj_add_event_cb(orientation_dropdown, onOrientationSet, LV_EVENT_VALUE_CHANGED, this);
        auto orientation = settings::display::toLvglDisplayRotation(displaySettings.orientation);
        lv_dropdown_set_selected(orientation_dropdown, orientation);
    }

    void onHide(TT_UNUSED AppContext& app) override {
        if (displaySettingsUpdated) {
            // Dispatch it, so file IO doesn't block the UI
            const settings::display::DisplaySettings settings_to_save = displaySettings;
            getMainDispatcher().dispatch([settings_to_save] {
                settings::display::save(settings_to_save);
            });
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
