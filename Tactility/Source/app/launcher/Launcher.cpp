#include <Tactility/Tactility.h>

#include <Tactility/app/AppContext.h>
#include <Tactility/app/AppPaths.h>
#include <Tactility/app/AppRegistration.h>
#include <Tactility/hal/power/PowerDevice.h>
#include <Tactility/service/loader/Loader.h>
#include <Tactility/settings/BootSettings.h>

#include <lvgl.h>

namespace tt::app::launcher {

constexpr auto* TAG = "Launcher";

static int getButtonSize(hal::UiScale scale) {
    if (scale == hal::UiScale::Smallest) {
        return 40;
    } else {
        return 64;
    }
}

class LauncherApp final : public App {

    static lv_obj_t* createAppButton(lv_obj_t* parent, hal::UiScale uiScale, const char* imageFile, const char* appId, int32_t horizontalMargin) {
        auto button_size = getButtonSize(uiScale);

        auto* apps_button = lv_button_create(parent);
        lv_obj_set_style_pad_all(apps_button, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_margin_hor(apps_button, horizontalMargin, LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(apps_button, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(apps_button, 0, LV_STATE_DEFAULT);

        auto* button_image = lv_image_create(apps_button);
        lv_image_set_src(button_image, imageFile);
        lv_obj_set_style_image_recolor(button_image, lv_theme_get_color_primary(parent), LV_STATE_DEFAULT);
        lv_obj_set_style_image_recolor_opa(button_image, LV_OPA_COVER, LV_STATE_DEFAULT);
        // Ensure buttons are still tappable when the asset fails to load
        // Icon images are 40x40, so we get some extra padding too
        lv_obj_set_size(button_image, button_size, button_size);

        lv_obj_add_event_cb(apps_button, onAppPressed, LV_EVENT_SHORT_CLICKED, (void*)appId);

        return apps_button;
    }

    static bool shouldShowPowerButton() {
        bool show_power_button = false;
        hal::findDevices<hal::power::PowerDevice>(hal::Device::Type::Power, [&show_power_button](const auto& device) {
            if (device->supportsPowerOff()) {
                show_power_button = true;
                return false; // stop iterating
            } else {
                return true; // continue iterating
            }
        });
        return show_power_button;
    }

    static void onAppPressed(TT_UNUSED lv_event_t* e) {
        auto* appId = static_cast<const char*>(lv_event_get_user_data(e));
        start(appId);
    }

    static void onPowerOffPressed(lv_event_t* e) {
        auto power = hal::findFirstDevice<hal::power::PowerDevice>(hal::Device::Type::Power);
        if (power != nullptr && power->supportsPowerOff()) {
            power->powerOff();
        }
    }

public:

    void onCreate(TT_UNUSED AppContext& app) override {
        settings::BootSettings boot_properties;
        if (settings::loadBootSettings(boot_properties) && !boot_properties.autoStartAppId.empty()) {
            TT_LOG_I(TAG, "Starting %s", boot_properties.autoStartAppId.c_str());
            start(boot_properties.autoStartAppId);
        }
    }

    void onShow(TT_UNUSED AppContext& app, lv_obj_t* parent) override {
        auto* buttons_wrapper = lv_obj_create(parent);

        auto ui_scale = hal::getConfiguration()->uiScale;
        auto button_size = getButtonSize(ui_scale);

        lv_obj_align(buttons_wrapper, LV_ALIGN_CENTER, 0, 0);
        // lv_obj_set_style_pad_all(buttons_wrapper, 0, LV_STATE_DEFAULT);
        lv_obj_set_size(buttons_wrapper, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_border_width(buttons_wrapper, 0, LV_STATE_DEFAULT);
        lv_obj_set_flex_grow(buttons_wrapper, 1);

        // Fix for button selection (problem with UiScale::Small on Cardputer)
        if (!hal::hasDevice(hal::Device::Type::Touch)) {
            lv_obj_set_style_pad_all(buttons_wrapper, 6, LV_STATE_DEFAULT);
        } else {
            lv_obj_set_style_pad_all(buttons_wrapper, 0, LV_STATE_DEFAULT);
        }

        const auto* display = lv_obj_get_display(parent);
        const auto horizontal_px = lv_display_get_horizontal_resolution(display);
        const auto vertical_px = lv_display_get_vertical_resolution(display);
        const bool is_landscape_display = horizontal_px >= vertical_px;
        if (is_landscape_display) {
            lv_obj_set_flex_flow(buttons_wrapper, LV_FLEX_FLOW_ROW);
        } else {
            lv_obj_set_flex_flow(buttons_wrapper, LV_FLEX_FLOW_COLUMN);
        }

        const int32_t available_width = lv_display_get_horizontal_resolution(display) - (3 * button_size);
        const int32_t margin = is_landscape_display ? std::min<int32_t>(available_width / 16, button_size) : 0;

        const auto paths = app.getPaths();
        const auto apps_icon_path = "A:" + paths->getAssetsPath("icon_apps.png");
        const auto files_icon_path = "A:" + paths->getAssetsPath("icon_files.png");
        const auto settings_icon_path = "A:" + paths->getAssetsPath("icon_settings.png");

        createAppButton(buttons_wrapper, ui_scale, apps_icon_path.c_str(), "AppList", margin);
        createAppButton(buttons_wrapper, ui_scale, files_icon_path.c_str(), "Files", margin);
        createAppButton(buttons_wrapper, ui_scale, settings_icon_path.c_str(), "Settings", margin);

        if (shouldShowPowerButton()) {
            auto* power_button = lv_btn_create(parent);
            lv_obj_set_style_pad_all(power_button, 8, 0);
            lv_obj_align(power_button, LV_ALIGN_BOTTOM_MID, 0, -10);
            lv_obj_add_event_cb(power_button, onPowerOffPressed, LV_EVENT_SHORT_CLICKED, nullptr);
            lv_obj_set_style_shadow_width(power_button, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(power_button, 0, LV_PART_MAIN);

            auto* power_label = lv_label_create(power_button);
            lv_label_set_text(power_label, LV_SYMBOL_POWER);
            lv_obj_set_style_text_color(power_label, lv_theme_get_color_primary(parent), LV_STATE_DEFAULT);
        }
    }
};

extern const AppManifest manifest = {
    .appId = "Launcher",
    .appName = "Launcher",
    .appCategory = Category::System,
    .appFlags = AppManifest::Flags::Hidden,
    .createApp = create<LauncherApp>
};

LaunchId start() {
    return app::start(manifest.appId);
}

} // namespace
