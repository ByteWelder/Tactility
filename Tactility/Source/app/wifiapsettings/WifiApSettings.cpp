#include "WifiApSettings.h"
#include "TactilityCore.h"
#include "app/App.h"
#include "lvgl.h"
#include "lvgl/Style.h"
#include "lvgl/Toolbar.h"
#include "service/loader/Loader.h"
#include "service/wifi/WifiSettings.h"

namespace tt::app::wifiapsettings {

#define TAG "wifi_ap_settings"

extern const Manifest manifest;

void start(const std::string& ssid) {
    Bundle bundle;
    bundle.putString("ssid", ssid);
    service::loader::startApp(manifest.id, false, bundle);
}

static void onToggleAutoConnect(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);
    const Bundle& parameters = *(const Bundle*)lv_event_get_user_data(event);
    if (code == LV_EVENT_VALUE_CHANGED) {
        auto* enable_switch = static_cast<lv_obj_t*>(lv_event_get_target(event));
        bool is_on = lv_obj_has_state(enable_switch, LV_STATE_CHECKED);
        std::string ssid = parameters.getString("ssid");

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

static void onShow(App& app, lv_obj_t* parent) {
    const Bundle& bundle = app.getParameters();
    std::string ssid = bundle.getString("ssid");

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lvgl::toolbar_create(parent, ssid);

    // Wrappers

    lv_obj_t* secondary_flex = lv_obj_create(parent);
    lv_obj_set_width(secondary_flex, LV_PCT(100));
    lv_obj_set_flex_grow(secondary_flex, 1);
    lv_obj_set_flex_flow(secondary_flex, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_border_width(secondary_flex, 0, 0);
    lvgl::obj_set_style_no_padding(secondary_flex);
    lvgl::obj_set_style_bg_invisible(secondary_flex);

    // align() methods don't work on flex, so we need this extra wrapper
    lv_obj_t* wrapper = lv_obj_create(secondary_flex);
    lv_obj_set_size(wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lvgl::obj_set_style_bg_invisible(wrapper);
    lv_obj_set_style_border_width(wrapper, 0, 0);

    // Auto-connect toggle

    lv_obj_t* auto_connect_label = lv_label_create(wrapper);
    lv_label_set_text(auto_connect_label, "Auto-connect");
    lv_obj_align(auto_connect_label, LV_ALIGN_TOP_LEFT, 0, 6);

    lv_obj_t* auto_connect_switch = lv_switch_create(wrapper);
    lv_obj_add_event_cb(auto_connect_switch, onToggleAutoConnect, LV_EVENT_VALUE_CHANGED, (void*)&bundle);
    lv_obj_align(auto_connect_switch, LV_ALIGN_TOP_RIGHT, 0, 0);

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

extern const Manifest manifest = {
    .id = "WifiApSettings",
    .name = "Wi-Fi AP Settings",
    .icon = LV_SYMBOL_WIFI,
    .type = TypeHidden,
    .onShow = onShow
};

} // namespace

