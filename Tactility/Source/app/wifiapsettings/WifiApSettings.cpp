#include <Tactility/service/wifi/WifiApSettings.h>
#include <Tactility/service/wifi/Wifi.h>
#include <Tactility/app/App.h>
#include <Tactility/app/AppContext.h>
#include <Tactility/app/AppManifest.h>
#include <Tactility/app/alertdialog/AlertDialog.h>
#include <Tactility/lvgl/Style.h>
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/TactilityCore.h>

#include <lvgl.h>

namespace tt::app::wifiapsettings {

constexpr auto* TAG = "WifiApSettings";

extern const AppManifest manifest;

void start(const std::string& ssid) {
    auto bundle = std::make_shared<Bundle>();
    bundle->putString("ssid", ssid);
    app::start(manifest.id, bundle);
}

static void onPressForget(TT_UNUSED lv_event_t* event) {
    std::vector<std::string> choices = {
        "Yes",
        "No"
    };
    alertdialog::start("Confirmation", "Forget the Wi-Fi access point?", choices);
}

static void onToggleAutoConnect(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);

    auto app = getCurrentAppContext();
    auto parameters = app->getParameters();
    tt_check(parameters != nullptr, "Parameters missing");

    if (code == LV_EVENT_VALUE_CHANGED) {
        auto* enable_switch = static_cast<lv_obj_t*>(lv_event_get_target(event));
        bool is_on = lv_obj_has_state(enable_switch, LV_STATE_CHECKED);
        std::string ssid = parameters->getString("ssid");

        service::wifi::settings::WifiApSettings settings;
        if (service::wifi::settings::load(ssid.c_str(), settings)) {
            settings.autoConnect = is_on;
            if (!service::wifi::settings::save(settings)) {
                TT_LOG_E(TAG, "Failed to save settings");
            }
        } else {
            TT_LOG_E(TAG, "Failed to load settings");
        }
    }
}

class WifiApSettings : public App {

    void onShow(AppContext& app, lv_obj_t* parent) override {
        auto paremeters = app.getParameters();
        tt_check(paremeters != nullptr, "Parameters missing");
        std::string ssid = paremeters->getString("ssid");

        lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

        lvgl::toolbar_create(parent, ssid);

        // Wrappers

        auto* wrapper = lv_obj_create(parent);
        lv_obj_set_width(wrapper, LV_PCT(100));
        lv_obj_set_flex_grow(wrapper, 1);
        lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
        lvgl::obj_set_style_bg_invisible(wrapper);

        // Auto-connect toggle

        auto* auto_connect_wrapper = lv_obj_create(wrapper);
        lv_obj_set_size(auto_connect_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(auto_connect_wrapper, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_gap(auto_connect_wrapper, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(auto_connect_wrapper, 0, LV_STATE_DEFAULT);

        auto* auto_connect_label = lv_label_create(auto_connect_wrapper);
        lv_label_set_text(auto_connect_label, "Auto-connect");
        lv_obj_align(auto_connect_label, LV_ALIGN_TOP_LEFT, 0, 6);

        auto* auto_connect_switch = lv_switch_create(auto_connect_wrapper);
        lv_obj_add_event_cb(auto_connect_switch, onToggleAutoConnect, LV_EVENT_VALUE_CHANGED, &paremeters);
        lv_obj_align(auto_connect_switch, LV_ALIGN_TOP_RIGHT, 0, 0);

        auto* forget_button = lv_button_create(wrapper);
        lv_obj_set_width(forget_button, LV_PCT(100));
        lv_obj_align_to(forget_button, auto_connect_wrapper, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        lv_obj_add_event_cb(forget_button, onPressForget, LV_EVENT_SHORT_CLICKED, nullptr);
        auto* forget_button_label = lv_label_create(forget_button);
        lv_obj_align(forget_button_label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(forget_button_label, "Forget");

        service::wifi::settings::WifiApSettings settings;
        if (service::wifi::settings::load(ssid.c_str(), settings)) {
            if (settings.autoConnect) {
                lv_obj_add_state(auto_connect_switch, LV_STATE_CHECKED);
            } else {
                lv_obj_remove_state(auto_connect_switch, LV_STATE_CHECKED);
            }
        } else {
            TT_LOG_W(TAG, "No settings found");
            lv_obj_add_flag(forget_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(auto_connect_wrapper, LV_OBJ_FLAG_HIDDEN);
        }
    }

    void onResult(TT_UNUSED AppContext& appContext, TT_UNUSED LaunchId launchId, TT_UNUSED Result result, std::unique_ptr<Bundle> bundle) override {
        if (result == Result::Ok && bundle != nullptr) {
            auto index = alertdialog::getResultIndex(*bundle);
            if (index == 0) { // Yes
                auto parameters = appContext.getParameters();
                tt_check(parameters != nullptr, "Parameters missing");

                std::string ssid = parameters->getString("ssid");
                if (service::wifi::settings::remove(ssid.c_str())) {
                    TT_LOG_I(TAG, "Removed SSID");

                    if (
                        service::wifi::getRadioState() == service::wifi::RadioState::ConnectionActive &&
                        service::wifi::getConnectionTarget() == ssid
                    ) {
                        service::wifi::disconnect();
                    }

                    // Stop self
                    app::stop();
                } else {
                    TT_LOG_E(TAG, "Failed to remove SSID");
                }
            }
        }
    }
};

extern const AppManifest manifest = {
    .id = "WifiApSettings",
    .name = "Wi-Fi AP Settings",
    .icon = LV_SYMBOL_WIFI,
    .category = Category::System,
    .flags = AppManifest::Flags::Hidden,
    .createApp = create<WifiApSettings>
};

} // namespace

