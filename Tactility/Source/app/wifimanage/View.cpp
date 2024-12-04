#include "View.h"

#include "Log.h"
#include "State.h"
#include "service/statusbar/Statusbar.h"
#include "service/wifi/Wifi.h"
#include "lvgl/Style.h"
#include "lvgl/Toolbar.h"

#include <string>
#include <set>

namespace tt::app::wifimanage {

#define TAG "wifi_main_view"

static void on_enable_switch_changed(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);
    auto* enable_switch = static_cast<lv_obj_t*>(lv_event_get_target(event));
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool is_on = lv_obj_has_state(enable_switch, LV_STATE_CHECKED);
        auto* bindings = static_cast<Bindings*>(lv_event_get_user_data(event));
        bindings->onWifiToggled(is_on);
    }
}

static void on_enable_on_boot_switch_changed(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);
    auto* enable_switch = static_cast<lv_obj_t*>(lv_event_get_target(event));
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool is_on = lv_obj_has_state(enable_switch, LV_STATE_CHECKED);
        service::wifi::settings::setEnableOnBoot(is_on);
    }
}

static void on_disconnect_pressed(lv_event_t* event) {
    auto* bindings = static_cast<Bindings*>(lv_event_get_user_data(event));
    bindings->onDisconnect();
}

// region Secondary updates

static void connect(lv_event_t* event) {
    lv_obj_t* wrapper = lv_event_get_current_target_obj(event);
    // Assumes that the second child of the button is a label ... risky
    lv_obj_t* label = lv_obj_get_child(wrapper, 0);
    // We get the SSID from the button label because it's safer than alloc'ing
    // our own and passing it as the event data
    const char* ssid = lv_label_get_text(label);
    if (ssid != nullptr) {
        TT_LOG_I(TAG, "Clicked AP: %s", ssid);
        auto* bindings = (Bindings*)lv_event_get_user_data(event);
        bindings->onConnectSsid(ssid);
    }
}

static void showDetails(lv_event_t* event) {
    lv_obj_t* wrapper = lv_event_get_current_target_obj(event);
    // Hack: Get the hidden label with the ssid
    lv_obj_t* ssid_label = lv_obj_get_child(wrapper, 1);
    const char* ssid = lv_label_get_text(ssid_label);
    auto* bindings = (Bindings*)lv_event_get_user_data(event);
    bindings->onShowApSettings(ssid);
}

void View::createSsidListItem(Bindings* bindings, const service::wifi::WifiApRecord& record, bool isConnecting) {
    lv_obj_t* wrapper = lv_obj_create(networks_list);
    lv_obj_add_event_cb(wrapper, &connect, LV_EVENT_CLICKED, bindings);
    lv_obj_set_user_data(wrapper, bindings);
    lv_obj_set_size(wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lvgl::obj_set_style_no_padding(wrapper);
    lv_obj_set_style_margin_all(wrapper, 0, 0);
    lv_obj_set_style_border_width(wrapper, 0, 0);

    lv_obj_t* label = lv_label_create(wrapper);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_text(label, record.ssid.c_str());
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label, LV_PCT(70));

    lv_obj_t* info_wrapper = lv_obj_create(wrapper);
    lv_obj_set_style_pad_all(info_wrapper, 0, 0);
    lv_obj_set_style_margin_all(info_wrapper, 0, 0);
    lv_obj_set_size(info_wrapper, 36, 36);
    lv_obj_set_style_border_color(info_wrapper, lv_theme_get_color_primary(info_wrapper), 0);
    lv_obj_add_event_cb(info_wrapper, &showDetails, LV_EVENT_CLICKED, bindings);
    lv_obj_align(info_wrapper, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t* info_label = lv_label_create(info_wrapper);
    lv_label_set_text(info_label, "i");
    // Hack: Create a hidden label to store data and pass it to the callback
    lv_obj_t* ssid_label = lv_label_create(info_wrapper);
    lv_label_set_text(ssid_label, record.ssid.c_str());
    lv_obj_add_flag(ssid_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(info_label, lv_theme_get_color_primary(info_wrapper), 0);
    lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 0);

    if (isConnecting) {
        lv_obj_t* connecting_spinner = lv_spinner_create(wrapper);
        lv_obj_set_size(connecting_spinner, 40, 40);
        lv_spinner_set_anim_params(connecting_spinner, 1000, 60);
        lv_obj_set_style_pad_all(connecting_spinner, 4, 0);
        lv_obj_align_to(connecting_spinner, info_wrapper, LV_ALIGN_OUT_LEFT_MID, -8, 0);
    } else {
        const char* icon = service::statusbar::getWifiStatusIconForRssi(record.rssi, record.auth_mode != WIFI_AUTH_OPEN);
        lv_obj_t* image = lv_image_create(wrapper);
        lv_image_set_src(image, icon);
        lv_obj_align(image, LV_ALIGN_RIGHT_MID, -50, 0);
    }
}

void View::updateNetworkList(State* state, Bindings* bindings) {
    lv_obj_clean(networks_list);

    switch (state->getRadioState()) {
        case service::wifi::WIFI_RADIO_ON_PENDING:
        case service::wifi::WIFI_RADIO_ON:
        case service::wifi::WIFI_RADIO_CONNECTION_PENDING:
        case service::wifi::WIFI_RADIO_CONNECTION_ACTIVE: {

            std::string connection_target = service::wifi::getConnectionTarget();
            auto& ap_records = state->lockApRecords();

            bool is_connected = !connection_target.empty() &&
                state->getRadioState() == service::wifi::WIFI_RADIO_CONNECTION_ACTIVE;
            bool added_connected = false;
            if (is_connected) {
                if (!ap_records.empty()) {
                    for (auto &record : ap_records) {
                        if (record.ssid == connection_target) {
                            lv_list_add_text(networks_list, "Connected");
                            createSsidListItem(bindings, record, false);
                            added_connected = true;
                            break;
                        }
                    }
                }
            }

            lv_list_add_text(networks_list, "Other networks");
            std::set<std::string> used_ssids;
            if (!ap_records.empty()) {
                for (auto& record : ap_records) {
                    if (used_ssids.find(record.ssid) == used_ssids.end()) {
                        bool connection_target_match = (record.ssid == connection_target);
                        bool is_connecting = connection_target_match
                            && state->getRadioState() == service::wifi::WIFI_RADIO_CONNECTION_PENDING &&
                            !connection_target.empty();
                        bool skip = connection_target_match && added_connected;
                        if (!skip) {
                            createSsidListItem(bindings, record, is_connecting);
                        }
                        used_ssids.insert(record.ssid);
                    }
                }
                lv_obj_clear_flag(networks_list, LV_OBJ_FLAG_HIDDEN);
            } else if (state->isScanning()) {
                lv_obj_add_flag(networks_list, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_clear_flag(networks_list, LV_OBJ_FLAG_HIDDEN);
                lv_obj_t* label = lv_label_create(networks_list);
                lv_label_set_text(label, "No networks found.");
            }
            state->unlockApRecords();
            break;
        }
        case service::wifi::WIFI_RADIO_OFF_PENDING:
        case service::wifi::WIFI_RADIO_OFF: {
            lv_obj_add_flag(networks_list, LV_OBJ_FLAG_HIDDEN);
            break;
        }
    }
}

void View::updateScanning(State* state) {
    if (state->getRadioState() == service::wifi::WIFI_RADIO_ON && state->isScanning()) {
        lv_obj_clear_flag(scanning_spinner, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(scanning_spinner, LV_OBJ_FLAG_HIDDEN);
    }
}

void View::updateWifiToggle(State* state) {
    lv_obj_clear_state(enable_switch, LV_STATE_ANY);
    switch (state->getRadioState()) {
        case service::wifi::WIFI_RADIO_ON:
        case service::wifi::WIFI_RADIO_CONNECTION_PENDING:
        case service::wifi::WIFI_RADIO_CONNECTION_ACTIVE:
            lv_obj_add_state(enable_switch, LV_STATE_CHECKED);
            break;
        case service::wifi::WIFI_RADIO_ON_PENDING:
            lv_obj_add_state(enable_switch, LV_STATE_CHECKED | LV_STATE_DISABLED);
            break;
        case service::wifi::WIFI_RADIO_OFF:
            lv_obj_remove_state(enable_switch, LV_STATE_CHECKED | LV_STATE_DISABLED);
            break;
        case service::wifi::WIFI_RADIO_OFF_PENDING:
            lv_obj_remove_state(enable_switch, LV_STATE_CHECKED);
            lv_obj_add_state(enable_switch, LV_STATE_DISABLED);
            break;
    }
}

void View::updateEnableOnBootToggle() {
    lv_obj_clear_state(enable_on_boot_switch, LV_STATE_ANY);
    if (service::wifi::settings::shouldEnableOnBoot()) {
        lv_obj_add_state(enable_on_boot_switch, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(enable_on_boot_switch, LV_STATE_CHECKED);
    }
}

// endregion Secondary updates

// region Main

void View::init(const App& app, Bindings* bindings, lv_obj_t* parent) {
    root = parent;

    // Toolbar
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_t* toolbar = lvgl::toolbar_create(parent, app);

    scanning_spinner = lvgl::toolbar_add_spinner_action(toolbar);

    enable_switch = lvgl::toolbar_add_switch_action(toolbar);
    lv_obj_add_event_cb(enable_switch, on_enable_switch_changed, LV_EVENT_VALUE_CHANGED, bindings);

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

    // Enable on boot

    lv_obj_t* enable_label = lv_label_create(wrapper);
    lv_label_set_text(enable_label, "Enable on boot");
    lv_obj_align(enable_label, LV_ALIGN_TOP_LEFT, 0, 6);

    enable_on_boot_switch = lv_switch_create(wrapper);
    lv_obj_add_event_cb(enable_on_boot_switch, on_enable_on_boot_switch_changed, LV_EVENT_VALUE_CHANGED, bindings);
    lv_obj_align(enable_on_boot_switch, LV_ALIGN_TOP_RIGHT, 0, 0);

    // Networks

    networks_list = lv_obj_create(wrapper);
    lv_obj_set_flex_flow(networks_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(networks_list, LV_PCT(100));
    lv_obj_set_height(networks_list, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_top(networks_list, 0, 0);
    lv_obj_set_style_pad_bottom(networks_list, 0, 0);
    lv_obj_align(networks_list, LV_ALIGN_TOP_LEFT, 0, 44);
}

void View::update(Bindings* bindings, State* state) {
    updateWifiToggle(state);
    updateEnableOnBootToggle();
    updateScanning(state);
    updateNetworkList(state, bindings);
}

} // namespace
