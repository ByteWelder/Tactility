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
#define SPINNER_HEIGHT 40

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
    lv_obj_t* button = lv_event_get_current_target_obj(event);
    // Assumes that the second child of the button is a label ... risky
    lv_obj_t* label = lv_obj_get_child(button, 1);
    // We get the SSID from the button label because it's safer than alloc'ing
    // our own and passing it as the event data
    const char* ssid = lv_label_get_text(label);
    TT_LOG_I(TAG, "Clicked AP: %s", ssid);
    auto* bindings = static_cast<Bindings*>(lv_event_get_user_data(event));
    bindings->onConnectSsid(ssid);
}

void View::createNetworkButton(Bindings* bindings, const service::wifi::WifiApRecord& record) {
    const char* icon = service::statusbar::getWifiStatusIconForRssi(record.rssi, record.auth_mode != WIFI_AUTH_OPEN);
    lv_obj_t* ap_button = lv_list_add_btn(
        networks_list,
        icon,
        record.ssid.c_str()
    );
    lv_obj_add_event_cb(ap_button, &connect, LV_EVENT_CLICKED, bindings);
}

void View::updateNetworkList(State* state, Bindings* bindings) {
    lv_obj_clean(networks_list);
    switch (state->getRadioState()) {
        case service::wifi::WIFI_RADIO_ON_PENDING:
        case service::wifi::WIFI_RADIO_ON:
        case service::wifi::WIFI_RADIO_CONNECTION_PENDING:
        case service::wifi::WIFI_RADIO_CONNECTION_ACTIVE: {
            lv_obj_clear_flag(networks_label, LV_OBJ_FLAG_HIDDEN);
            auto& ap_records = state->lockApRecords();
            std::set<std::string> used_ssids;
            if (!ap_records.empty()) {
                for (auto& record : ap_records) {
                    if (used_ssids.find(record.ssid) == used_ssids.end()) {
                        createNetworkButton(bindings, record);
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
            lv_obj_add_flag(networks_label, LV_OBJ_FLAG_HIDDEN);
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

void View::updateConnectedAp(State* state, TT_UNUSED Bindings* bindings) {
    switch (state->getRadioState()) {
        case service::wifi::WIFI_RADIO_CONNECTION_PENDING:
        case service::wifi::WIFI_RADIO_CONNECTION_ACTIVE:
            lv_obj_clear_flag(connected_ap_container, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(connected_ap_label, state->getConnectSsid().c_str());
            break;
        default:
            lv_obj_add_flag(connected_ap_container, LV_OBJ_FLAG_HIDDEN);
            break;
    }
}

// endregion Secondary updates

// region Main

void View::init(const App& app, Bindings* bindings, lv_obj_t* parent) {
    root = parent;

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_t* toolbar = lvgl::toolbar_create(parent, app);

    enable_switch = lvgl::toolbar_add_switch_action(toolbar);
    lv_obj_add_event_cb(enable_switch, on_enable_switch_changed, LV_EVENT_ALL, bindings);

    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);

    // Top row: enable/disable
    lv_obj_t* switch_container = lv_obj_create(wrapper);
    lv_obj_set_width(switch_container, LV_PCT(100));
    lv_obj_set_height(switch_container, LV_SIZE_CONTENT);
    lvgl::obj_set_style_no_padding(switch_container);
    lvgl::obj_set_style_bg_invisible(switch_container);

    lv_obj_t* enable_label = lv_label_create(switch_container);
    lv_label_set_text(enable_label, "Enable on boot");
    lv_obj_set_align(enable_label, LV_ALIGN_LEFT_MID);

    enable_on_boot_switch = lv_switch_create(switch_container);
    lv_obj_add_event_cb(enable_on_boot_switch, on_enable_on_boot_switch_changed, LV_EVENT_ALL, bindings);
    lv_obj_set_align(enable_on_boot_switch, LV_ALIGN_RIGHT_MID);

    connected_ap_container = lv_obj_create(wrapper);
    lv_obj_set_size(connected_ap_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(connected_ap_container, SPINNER_HEIGHT, 0);
    lvgl::obj_set_style_no_padding(connected_ap_container);
    lv_obj_set_style_border_width(connected_ap_container, 0, 0);

    connected_ap_label = lv_label_create(connected_ap_container);
    lv_obj_align(connected_ap_label, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* disconnect_button = lv_btn_create(connected_ap_container);
    lv_obj_add_event_cb(disconnect_button, &on_disconnect_pressed, LV_EVENT_CLICKED, bindings);
    lv_obj_t* disconnect_label = lv_label_create(disconnect_button);
    lv_label_set_text(disconnect_label, "Disconnect");
    lv_obj_align(disconnect_button, LV_ALIGN_RIGHT_MID, 0, 0);

    // Networks

    lv_obj_t* networks_header = lv_obj_create(wrapper);
    lv_obj_set_size(networks_header, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(networks_header, SPINNER_HEIGHT, 0);
    lvgl::obj_set_style_no_padding(networks_header);
    lv_obj_set_style_border_width(networks_header, 0, 0);

    networks_label = lv_label_create(networks_header);
    lv_label_set_text(networks_label, "Networks");
    lv_obj_align(networks_label, LV_ALIGN_LEFT_MID, 0, 0);

    scanning_spinner = lv_spinner_create(networks_header);
    lv_spinner_set_anim_params(scanning_spinner, 1000, 60);
    lv_obj_set_size(scanning_spinner, SPINNER_HEIGHT, SPINNER_HEIGHT);
    lv_obj_set_style_pad_top(scanning_spinner, 4, 0);
    lv_obj_set_style_pad_bottom(scanning_spinner, 4, 0);
    lv_obj_align_to(scanning_spinner, networks_label, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    networks_list = lv_obj_create(wrapper);
    lv_obj_set_flex_flow(networks_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(networks_list, LV_PCT(100));
    lv_obj_set_height(networks_list, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_top(networks_list, 8, 0);
    lv_obj_set_style_pad_bottom(networks_list, 8, 0);
}

void View::update(Bindings* bindings, State* state) {
    updateWifiToggle(state);
    updateEnableOnBootToggle();
    updateScanning(state);
    updateNetworkList(state, bindings);
    updateConnectedAp(state, bindings);
}

} // namespace
