#include <lvgl.h>

#include <lvgl/devices/device_context.h>
#include <lvgl/devices/trackball.h>
#include <lvgl/icons/shared.h>
#include <lvgl/lvgl.h>

#include <tactility/drivers/trackball.h>

#include <Tactility/Assets.h>
#include <Tactility/settings/TrackballSettings.h>
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/Tactility.h>

namespace tt::app::trackballsettings {

constexpr auto* TAG = "TrackballSettings";

// Convert mode to dropdown index (dropdown order: Encoder=0, Pointer=1)
static uint32_t modeToDropdownIndex(LvglTrackballMode mode) {
    switch (mode) {
        case LVGL_TRACKBALL_MODE_ENCODER: return 0;
        case LVGL_TRACKBALL_MODE_POINTER: return 1;
    }
    return 0; // default to Encoder
}

class TrackballSettingsApp final : public App {

    LvglTrackballSettings tbSettings;
    bool updated = false;
    // The trackball indev currently bound by lvgl_devices_attach() at LVGL startup, if any -
    // there's at most one at a time (see devices.c), so "first active trackball device" reduces
    // to whatever is already attached.
    lv_indev_t* trackballIndev = nullptr;
    lv_obj_t* switchTrackball = nullptr;
    lv_obj_t* trackballModeDropdown = nullptr;
    lv_obj_t* encoderSensitivitySlider = nullptr;
    lv_obj_t* pointerSensitivitySlider = nullptr;

    void applyLive() {
        if (trackballIndev == nullptr) {
            return;
        }
        lvgl_lock();
        lvgl_trackball_set_settings(trackballIndev, &tbSettings);
        if (tbSettings.mode == LVGL_TRACKBALL_MODE_POINTER) {
            lvgl_trackball_set_cursor_image(trackballIndev, TT_ASSETS_UI_CURSOR);
        }
        lvgl_unlock();
    }

    static void onTrackballSwitch(lv_event_t* e) {
        auto* app = static_cast<TrackballSettingsApp*>(lv_event_get_user_data(e));
        bool enabled = lv_obj_has_state(app->switchTrackball, LV_STATE_CHECKED);
        app->tbSettings.enabled = enabled;
        app->updated = true;
        app->applyLive();

        // Enable/disable controls based on trackball state
        if (enabled) {
            if (app->trackballModeDropdown) lv_obj_clear_state(app->trackballModeDropdown, LV_STATE_DISABLED);
            if (app->encoderSensitivitySlider) lv_obj_clear_state(app->encoderSensitivitySlider, LV_STATE_DISABLED);
            if (app->pointerSensitivitySlider) lv_obj_clear_state(app->pointerSensitivitySlider, LV_STATE_DISABLED);
        } else {
            if (app->trackballModeDropdown) lv_obj_add_state(app->trackballModeDropdown, LV_STATE_DISABLED);
            if (app->encoderSensitivitySlider) lv_obj_add_state(app->encoderSensitivitySlider, LV_STATE_DISABLED);
            if (app->pointerSensitivitySlider) lv_obj_add_state(app->pointerSensitivitySlider, LV_STATE_DISABLED);
        }
    }

    static void onTrackballModeChanged(lv_event_t* e) {
        auto* app = static_cast<TrackballSettingsApp*>(lv_event_get_user_data(e));
        uint32_t selected = lv_dropdown_get_selected(app->trackballModeDropdown);

        // Validate selection matches expected enum values (dropdown order: Encoder=0, Pointer=1)
        LvglTrackballMode mode;
        switch (selected) {
            case 0: mode = LVGL_TRACKBALL_MODE_ENCODER; break;
            case 1: mode = LVGL_TRACKBALL_MODE_POINTER; break;
            default: return; // Invalid selection, ignore
        }

        app->tbSettings.mode = mode;
        app->updated = true;

        // Apply mode change immediately
        app->applyLive();
    }

    static void onEncoderSensitivityChanged(lv_event_t* e) {
        auto* app = static_cast<TrackballSettingsApp*>(lv_event_get_user_data(e));
        int32_t value = lv_slider_get_value(app->encoderSensitivitySlider);
        app->tbSettings.encoder_sensitivity = static_cast<uint8_t>(value);
        app->updated = true;

        // Apply immediately
        app->applyLive();
    }

    static void onPointerSensitivityChanged(lv_event_t* e) {
        auto* app = static_cast<TrackballSettingsApp*>(lv_event_get_user_data(e));
        int32_t value = lv_slider_get_value(app->pointerSensitivitySlider);
        app->tbSettings.pointer_sensitivity = static_cast<uint8_t>(value);
        app->updated = true;

        // Apply immediately
        app->applyLive();
    }

    static lv_indev_t* findFirstTrackballIndev() {
        lv_indev_t* indev = lv_indev_get_next(nullptr);
        while (indev != nullptr) {
            void* driver_data = lv_indev_get_driver_data(indev);
            if (driver_data) {
                LvglDeviceContext* context = static_cast<LvglDeviceContext*>(driver_data);
                if (context->device) {
                    const DeviceType* device_type = device_get_type(context->device);
                    if (device_type == &TRACKBALL_TYPE) {
                        return indev;
                    }
                }
            }

            indev = lv_indev_get_next(indev);
        }
        return nullptr;
    }

public:
    void onShow(AppContext& app, lv_obj_t* parent) override {
        tbSettings = settings::trackball::loadOrGetDefault();
        auto ui_density = lvgl_get_ui_density();
        updated = false;
        trackballIndev = findFirstTrackballIndev();

        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

        lv_obj_t* toolbar = lvgl::toolbar_create(parent, app);

        if (trackballIndev == nullptr) {
            auto* wrapper = lv_obj_create(parent);
            lv_obj_set_width(wrapper, LV_PCT(100));
            lv_obj_set_flex_grow(wrapper, 1);
            lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            auto* label = lv_label_create(wrapper);
            lv_label_set_text(label, "No trackball device found");
            return;
        }

        // The live indev may still be running with lvgl_trackball_settings_get_default() (it's
        // bound at LVGL startup before persisted settings are known) - bring it in line with what
        // this screen is about to display.
        applyLive();

        switchTrackball = lvgl_toolbar_add_switch_action(toolbar);
        lv_obj_add_event_cb(switchTrackball, onTrackballSwitch, LV_EVENT_VALUE_CHANGED, this);
        if (tbSettings.enabled) lv_obj_add_state(switchTrackball, LV_STATE_CHECKED);

        auto* main_wrapper = lv_obj_create(parent);
        lv_obj_set_flex_flow(main_wrapper, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_width(main_wrapper, LV_PCT(100));
        lv_obj_set_flex_grow(main_wrapper, 1);

        // Trackball mode dropdown
        auto* tb_mode_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_size(tb_mode_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(tb_mode_wrapper, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(tb_mode_wrapper, 0, LV_STATE_DEFAULT);

        auto* tb_mode_label = lv_label_create(tb_mode_wrapper);
        lv_label_set_text(tb_mode_label, "Mode");
        lv_obj_align(tb_mode_label, LV_ALIGN_LEFT_MID, 0, 0);

        trackballModeDropdown = lv_dropdown_create(tb_mode_wrapper);
        lv_dropdown_set_options(trackballModeDropdown, "Encoder\nPointer");
        lv_obj_align(trackballModeDropdown, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_dropdown_set_selected(trackballModeDropdown, modeToDropdownIndex(tbSettings.mode));
        lv_obj_add_event_cb(trackballModeDropdown, onTrackballModeChanged, LV_EVENT_VALUE_CHANGED, this);

        // Disable dropdown if trackball is disabled
        if (!tbSettings.enabled) {
            lv_obj_add_state(trackballModeDropdown, LV_STATE_DISABLED);
        }

        // Encoder sensitivity slider
        auto* enc_sens_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_size(enc_sens_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_hor(enc_sens_wrapper, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(enc_sens_wrapper, 0, LV_STATE_DEFAULT);
        if (ui_density != LVGL_UI_DENSITY_COMPACT) {
            lv_obj_set_style_pad_ver(enc_sens_wrapper, 4, LV_STATE_DEFAULT);
        }

        auto* enc_sens_label = lv_label_create(enc_sens_wrapper);
        lv_label_set_text(enc_sens_label, "Encoder Speed");
        lv_obj_align(enc_sens_label, LV_ALIGN_LEFT_MID, 0, 0);

        encoderSensitivitySlider = lv_slider_create(enc_sens_wrapper);
        lv_slider_set_range(encoderSensitivitySlider, 1, 10);
        lv_slider_set_value(encoderSensitivitySlider, tbSettings.encoder_sensitivity, LV_ANIM_OFF);
        lv_obj_set_width(encoderSensitivitySlider, LV_PCT(50));
        lv_obj_align(encoderSensitivitySlider, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_add_event_cb(encoderSensitivitySlider, onEncoderSensitivityChanged, LV_EVENT_VALUE_CHANGED, this);

        if (!tbSettings.enabled) {
            lv_obj_add_state(encoderSensitivitySlider, LV_STATE_DISABLED);
        }

        // Pointer sensitivity slider
        auto* ptr_sens_wrapper = lv_obj_create(main_wrapper);
        lv_obj_set_size(ptr_sens_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_hor(ptr_sens_wrapper, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ptr_sens_wrapper, 0, LV_STATE_DEFAULT);
        if (ui_density != LVGL_UI_DENSITY_COMPACT) {
            lv_obj_set_style_pad_ver(ptr_sens_wrapper, 4, LV_STATE_DEFAULT);
        }

        auto* ptr_sens_label = lv_label_create(ptr_sens_wrapper);
        lv_label_set_text(ptr_sens_label, "Pointer Speed");
        lv_obj_align(ptr_sens_label, LV_ALIGN_LEFT_MID, 0, 0);

        pointerSensitivitySlider = lv_slider_create(ptr_sens_wrapper);
        lv_slider_set_range(pointerSensitivitySlider, 1, 10);
        lv_slider_set_value(pointerSensitivitySlider, tbSettings.pointer_sensitivity, LV_ANIM_OFF);
        lv_obj_set_width(pointerSensitivitySlider, LV_PCT(50));
        lv_obj_align(pointerSensitivitySlider, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_add_event_cb(pointerSensitivitySlider, onPointerSensitivityChanged, LV_EVENT_VALUE_CHANGED, this);

        if (!tbSettings.enabled) {
            lv_obj_add_state(pointerSensitivitySlider, LV_STATE_DISABLED);
        }
    }

    void onHide(AppContext& app) override {
        if (updated) {
            const auto copy = tbSettings;
            getMainDispatcher().dispatch([copy]{ settings::trackball::save(copy); });
            updated = false;
        }
    }
};

extern const AppManifest manifest = {
    .appId = "TrackballSettings",
    .appName = "Trackball",
    .appIcon = LVGL_ICON_SHARED_CIRCLE,
    .appCategory = Category::Settings,
    .createApp = create<TrackballSettingsApp>
};

}
