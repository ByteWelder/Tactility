#include <Tactility/Tactility.h>

#include <Tactility/settings/KeyboardSettings.h>
#include <Tactility/Assets.h>
#include <Tactility/lvgl/Toolbar.h>

#include <lvgl.h>

// Forward declare driver functions
namespace driver::keyboardbacklight {
    bool setBrightness(uint8_t brightness);
}

namespace driver::trackball {
    void setEnabled(bool enabled);
}

namespace tt::app::keyboardsettings {

constexpr auto* TAG = "KeyboardSettings";

static void applyKeyboardBacklight(bool enabled, uint8_t brightness) {
    driver::keyboardbacklight::setBrightness(enabled ? brightness : 0);
}

class KeyboardSettingsApp final : public App {

    settings::keyboard::KeyboardSettings kbSettings;
    bool updated = false;
    lv_obj_t* switchBacklight = nullptr;
    lv_obj_t* sliderBrightness = nullptr;
    lv_obj_t* switchTrackball = nullptr;
    lv_obj_t* switchTimeoutEnable = nullptr;
    lv_obj_t* timeoutDropdown = nullptr;

    static void onBacklightSwitch(lv_event_t* e) {
        auto* app = static_cast<KeyboardSettingsApp*>(lv_event_get_user_data(e));
        bool enabled = lv_obj_has_state(app->switchBacklight, LV_STATE_CHECKED);
        app->kbSettings.backlightEnabled = enabled;
        app->updated = true;
        if (app->sliderBrightness) {
            if (enabled) lv_obj_clear_state(app->sliderBrightness, LV_STATE_DISABLED);
            else lv_obj_add_state(app->sliderBrightness, LV_STATE_DISABLED);
        }
        applyKeyboardBacklight(enabled, app->kbSettings.backlightBrightness);
    }

    static void onBrightnessChanged(lv_event_t* e) {
        auto* app = static_cast<KeyboardSettingsApp*>(lv_event_get_user_data(e));
        int32_t v = lv_slider_get_value(app->sliderBrightness);
        app->kbSettings.backlightBrightness = static_cast<uint8_t>(v);
        app->updated = true;
        if (app->kbSettings.backlightEnabled) {
            applyKeyboardBacklight(true, app->kbSettings.backlightBrightness);
        }
    }

    static void onTrackballSwitch(lv_event_t* e) {
        auto* app = static_cast<KeyboardSettingsApp*>(lv_event_get_user_data(e));
        bool enabled = lv_obj_has_state(app->switchTrackball, LV_STATE_CHECKED);
        app->kbSettings.trackballEnabled = enabled;
        app->updated = true;
        driver::trackball::setEnabled(enabled);
    }

    static void onTimeoutEnableSwitch(lv_event_t* e) {
        auto* app = static_cast<KeyboardSettingsApp*>(lv_event_get_user_data(e));
        bool enabled = lv_obj_has_state(app->switchTimeoutEnable, LV_STATE_CHECKED);
        app->kbSettings.backlightTimeoutEnabled = enabled;
        app->updated = true;
        if (app->timeoutDropdown) {
            if (enabled) {
                lv_obj_clear_state(app->timeoutDropdown, LV_STATE_DISABLED);
            } else {
                lv_obj_add_state(app->timeoutDropdown, LV_STATE_DISABLED);
            }
        }
    }

    static void onTimeoutChanged(lv_event_t* event) {
        auto* app = static_cast<KeyboardSettingsApp*>(lv_event_get_user_data(event));
        auto* dropdown = static_cast<lv_obj_t*>(lv_event_get_target(event));
        uint32_t idx = lv_dropdown_get_selected(dropdown);
        // Map dropdown index to ms: 0=15s,1=30s,2=1m,3=2m,4=5m,5=Never
        static const uint32_t values_ms[] = {15000, 30000, 60000, 120000, 300000, 0};
        if (idx < (sizeof(values_ms)/sizeof(values_ms[0]))) {
            app->kbSettings.backlightTimeoutMs = values_ms[idx];
        app->updated = true;
        }
    }

public:
    void onShow(AppContext& app, lv_obj_t* parent) override {
        kbSettings = settings::keyboard::loadOrGetDefault();

        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

        lvgl::toolbar_create(parent, app);

        auto* main_wrapper = lv_obj_create(parent);
        lv_obj_set_flex_flow(main_wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_width(main_wrapper, LV_PCT(100));
        lv_obj_set_flex_grow(main_wrapper, 1);

        // Keyboard backlight toggle
        auto* bl_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_size(bl_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(bl_wrapper, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(bl_wrapper, 0, LV_STATE_DEFAULT);

        auto* bl_label = lv_label_create(bl_wrapper);
        lv_label_set_text(bl_label, "Keyboard backlight");
        lv_obj_align(bl_label, LV_ALIGN_LEFT_MID, 0, 0);
        switchBacklight = lv_switch_create(bl_wrapper);
        if (kbSettings.backlightEnabled) lv_obj_add_state(switchBacklight, LV_STATE_CHECKED);
        lv_obj_align(switchBacklight, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_add_event_cb(switchBacklight, onBacklightSwitch, LV_EVENT_VALUE_CHANGED, this);

        // Brightness slider
        auto* br_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_size(br_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(br_wrapper, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(br_wrapper, 0, LV_STATE_DEFAULT);

        auto* br_label = lv_label_create(br_wrapper);
        lv_label_set_text(br_label, "Brightness");
        lv_obj_align(br_label, LV_ALIGN_LEFT_MID, 0, 0);
        sliderBrightness = lv_slider_create(br_wrapper);
        lv_obj_set_width(sliderBrightness, LV_PCT(50));
        lv_obj_align(sliderBrightness, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_slider_set_range(sliderBrightness, 0, 255);
        lv_slider_set_value(sliderBrightness, kbSettings.backlightBrightness, LV_ANIM_OFF);
        if (!kbSettings.backlightEnabled) lv_obj_add_state(sliderBrightness, LV_STATE_DISABLED);
        lv_obj_add_event_cb(sliderBrightness, onBrightnessChanged, LV_EVENT_VALUE_CHANGED, this);

        // Trackball toggle
        auto* tb_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_size(tb_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(tb_wrapper, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(tb_wrapper, 0, LV_STATE_DEFAULT);

        auto* tb_label = lv_label_create(tb_wrapper);
        lv_label_set_text(tb_label, "Trackball");
        lv_obj_align(tb_label, LV_ALIGN_LEFT_MID, 0, 0);
        switchTrackball = lv_switch_create(tb_wrapper);
        if (kbSettings.trackballEnabled) lv_obj_add_state(switchTrackball, LV_STATE_CHECKED);
        lv_obj_align(switchTrackball, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_add_event_cb(switchTrackball, onTrackballSwitch, LV_EVENT_VALUE_CHANGED, this);

        // Backlight timeout enable
        auto* to_enable_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_size(to_enable_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(to_enable_wrapper, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(to_enable_wrapper, 0, LV_STATE_DEFAULT);

        auto* to_enable_label = lv_label_create(to_enable_wrapper);
        lv_label_set_text(to_enable_label, "Auto backlight off");
        lv_obj_align(to_enable_label, LV_ALIGN_LEFT_MID, 0, 0);
        switchTimeoutEnable = lv_switch_create(to_enable_wrapper);
        if (kbSettings.backlightTimeoutEnabled) lv_obj_add_state(switchTimeoutEnable, LV_STATE_CHECKED);
        lv_obj_align(switchTimeoutEnable, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_add_event_cb(switchTimeoutEnable, onTimeoutEnableSwitch, LV_EVENT_VALUE_CHANGED, this);

        auto* timeout_select_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_size(timeout_select_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(timeout_select_wrapper, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(timeout_select_wrapper, 0, LV_STATE_DEFAULT);

        auto* timeout_value_label = lv_label_create(timeout_select_wrapper);
        lv_label_set_text(timeout_value_label, "Timeout");
        lv_obj_align(timeout_value_label, LV_ALIGN_LEFT_MID, 0, 0);

        // Backlight timeout value (seconds)
        timeoutDropdown = lv_dropdown_create(timeout_select_wrapper);
        lv_dropdown_set_options(timeoutDropdown, "15 seconds\n30 seconds\n1 minute\n2 minutes\n5 minutes\nNever");
        lv_obj_align(timeoutDropdown, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_style_border_color(timeoutDropdown, lv_color_hex(0xFAFAFA), LV_PART_MAIN);
        lv_obj_set_style_border_width(timeoutDropdown, 1, LV_PART_MAIN);
        lv_obj_add_event_cb(timeoutDropdown, onTimeoutChanged, LV_EVENT_VALUE_CHANGED, this);
        // Initialize dropdown selection from settings
        uint32_t ms = kbSettings.backlightTimeoutMs;
        uint32_t idx = 2; // default 1 minute
        if (ms == 15000) idx = 0; else if (ms == 30000) idx = 1; else if (ms == 60000) idx = 2; else if (ms == 120000) idx = 3; else if (ms == 300000) idx = 4; else if (ms == 0) idx = 5;
        lv_dropdown_set_selected(timeoutDropdown, idx);
        if (!kbSettings.backlightTimeoutEnabled) {
            lv_obj_add_state(timeoutDropdown, LV_STATE_DISABLED);
        }
    }

    void onHide(TT_UNUSED AppContext& app) override {
        if (updated) {
            const auto copy = kbSettings;
            getMainDispatcher().dispatch([copy]{ settings::keyboard::save(copy); });
        }
    }
};

extern const AppManifest manifest = {
    .appId = "KeyboardSettings",
    .appName = "Keyboard",
    .appIcon = TT_ASSETS_APP_ICON_SETTINGS,
    .appCategory = Category::Settings,
    .createApp = create<KeyboardSettingsApp>
};

}
