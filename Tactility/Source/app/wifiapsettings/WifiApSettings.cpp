#include "WifiApSettings.h"
#include "TactilityCore.h"
#include "app/AppContext.h"
#include "app/alertdialog/AlertDialog.h"
#include "lvgl.h"
#include "lvgl/Style.h"
#include "lvgl/Toolbar.h"
#include "service/loader/Loader.h"
#include "service/wifi/WifiSettings.h"

namespace tt::app::wifiapsettings {

#define TAG "wifi_ap_settings"

extern const AppManifest manifest;

/** Returns the app data if the app is active. Note that this could clash if the same app is started twice and a background thread is slow. */
const AppContext* _Nullable optWifiApSettingsApp() {
    app::AppContext* app = service::loader::getCurrentApp();
    if (app->getManifest().id == manifest.id) {
        return app;
    } else {
        return nullptr;
    }
}

void start(const std::string& ssid) {
    auto bundle = std::make_shared<Bundle>();
    bundle->putString("ssid", ssid);
    service::loader::startApp(manifest.id, false, bundle);
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

    auto* app = optWifiApSettingsApp();
    if (app == nullptr) {
        return;
    }

    auto parameters = app->getParameters();
    tt_check(parameters != nullptr, "Parameters missing");

    if (code == LV_EVENT_VALUE_CHANGED) {
        auto* enable_switch = static_cast<lv_obj_t*>(lv_event_get_target(event));
        bool is_on = lv_obj_has_state(enable_switch, LV_STATE_CHECKED);
        std::string ssid = parameters->getString("ssid");

        service::wifi::settings::WifiApSettings settings {};
        if (service::wifi::settings::load(ssid.c_str(), &settings)) {
            settings.auto_connect = is_on;
            if (!service::wifi::settings::save(&settings)) {
                TT_LOG_E(TAG, "Failed to save settings");
            }
        } else {
            TT_LOG_E(TAG, "Failed to load settings");
        }
    }
}

static void onShow(AppContext& app, lv_obj_t* parent) {
    auto paremeters = app.getParameters();
    tt_check(paremeters != nullptr, "Parameters missing");
    std::string ssid = paremeters->getString("ssid");

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lvgl::toolbar_create(parent, ssid);

    // Wrappers

    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    lvgl::obj_set_style_bg_invisible(wrapper);

    // Auto-connect toggle

    lv_obj_t* auto_connect_wrapper = lv_obj_create(wrapper);
    lv_obj_set_size(auto_connect_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lvgl::obj_set_style_no_padding(auto_connect_wrapper);
    lv_obj_set_style_border_width(auto_connect_wrapper, 0, 0);

    lv_obj_t* auto_connect_label = lv_label_create(auto_connect_wrapper);
    lv_label_set_text(auto_connect_label, "Auto-connect");
    lv_obj_align(auto_connect_label, LV_ALIGN_TOP_LEFT, 0, 6);

    lv_obj_t* auto_connect_switch = lv_switch_create(auto_connect_wrapper);
    lv_obj_add_event_cb(auto_connect_switch, onToggleAutoConnect, LV_EVENT_VALUE_CHANGED, (void*)&paremeters);
    lv_obj_align(auto_connect_switch, LV_ALIGN_TOP_RIGHT, 0, 0);

    lv_obj_t* forget_button = lv_button_create(wrapper);
    lv_obj_set_width(forget_button, LV_PCT(100));
    lv_obj_align_to(forget_button, auto_connect_wrapper, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_add_event_cb(forget_button, onPressForget, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* forget_button_label = lv_label_create(forget_button);
    lv_obj_align(forget_button_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(forget_button_label, "Forget");

    service::wifi::settings::WifiApSettings settings {};
    if (service::wifi::settings::load(ssid.c_str(), &settings)) {
        if (settings.auto_connect) {
            lv_obj_add_state(auto_connect_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(auto_connect_switch, LV_STATE_CHECKED);
        }

    } else {
        TT_LOG_E(TAG, "Failed to load settings");
    }
}

void onResult(TT_UNUSED AppContext& app, TT_UNUSED Result result, const Bundle& bundle) {
    auto index = alertdialog::getResultIndex(bundle);
    if (index == 0) {// Yes
        auto* app = optWifiApSettingsApp();
        if (app == nullptr) {
            return;
        }

        auto parameters = app->getParameters();
        tt_check(parameters != nullptr, "Parameters missing");

        std::string ssid = parameters->getString("ssid");
        if (service::wifi::settings::remove(ssid.c_str())) {
            TT_LOG_I(TAG, "Removed SSID");

            if (
                service::wifi::getRadioState() == service::wifi::WIFI_RADIO_CONNECTION_ACTIVE &&
                service::wifi::getConnectionTarget() == ssid
            ) {
                service::wifi::disconnect();
            }

            // Stop self
            service::loader::stopApp();
        } else {
            TT_LOG_E(TAG, "Failed to remove SSID");
        }
    }
}

extern const AppManifest manifest = {
    .id = "WifiApSettings",
    .name = "Wi-Fi AP Settings",
    .icon = LV_SYMBOL_WIFI,
    .type = TypeHidden,
    .onShow = onShow,
    .onResult = onResult
};

} // namespace

