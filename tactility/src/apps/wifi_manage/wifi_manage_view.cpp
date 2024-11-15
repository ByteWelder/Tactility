#include "wifi_manage_view.h"

#include "log.h"
#include "services/statusbar_updater/statusbar_updater.h"
#include "services/wifi/wifi.h"
#include "ui/style.h"
#include "ui/toolbar.h"
#include "wifi_manage_state.h"

#define TAG "wifi_main_view"
#define SPINNER_HEIGHT 40

static void on_enable_switch_changed(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);
    auto* enable_switch = static_cast<lv_obj_t*>(lv_event_get_target(event));
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool is_on = lv_obj_has_state(enable_switch, LV_STATE_CHECKED);
        auto* bindings = static_cast<WifiManageBindings*>(lv_event_get_user_data(event));
        bindings->on_wifi_toggled(is_on);
    }
}

static void on_disconnect_pressed(lv_event_t* event) {
    auto* bindings = static_cast<WifiManageBindings*>(lv_event_get_user_data(event));
    bindings->on_disconnect();
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
    auto* bindings = static_cast<WifiManageBindings*>(lv_event_get_user_data(event));
    bindings->on_connect_ssid(ssid);
}

static void create_network_button(WifiManageView* view, WifiManageBindings* bindings, WifiApRecord* record) {
    const char* ssid = (const char*)record->ssid;
    const char* icon = wifi_get_status_icon_for_rssi(record->rssi, record->auth_mode != WIFI_AUTH_OPEN);
    lv_obj_t* ap_button = lv_list_add_btn(
        view->networks_list,
        icon,
        ssid
    );
    lv_obj_add_event_cb(ap_button, &connect, LV_EVENT_CLICKED, bindings);
}

static void update_network_list(WifiManageView* view, WifiManageState* state, WifiManageBindings* bindings) {
    lv_obj_clean(view->networks_list);
    switch (state->radio_state) {
        case WIFI_RADIO_ON_PENDING:
        case WIFI_RADIO_ON:
        case WIFI_RADIO_CONNECTION_PENDING:
        case WIFI_RADIO_CONNECTION_ACTIVE: {
            lv_obj_clear_flag(view->networks_label, LV_OBJ_FLAG_HIDDEN);
            if (state->ap_records_count > 0) {
                for (int i = 0; i < state->ap_records_count; ++i) {
                    create_network_button(view, bindings, &state->ap_records[i]);
                }
                lv_obj_clear_flag(view->networks_list, LV_OBJ_FLAG_HIDDEN);
            } else if (state->scanning) {
                lv_obj_add_flag(view->networks_list, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_clear_flag(view->networks_list, LV_OBJ_FLAG_HIDDEN);
                lv_obj_t* label = lv_label_create(view->networks_list);
                lv_label_set_text(label, "No networks found.");
            }
            break;
        }
        case WIFI_RADIO_OFF_PENDING:
        case WIFI_RADIO_OFF: {
            lv_obj_add_flag(view->networks_list, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(view->networks_label, LV_OBJ_FLAG_HIDDEN);
            break;
        }
    }
}

void update_scanning(WifiManageView* view, WifiManageState* state) {
    if (state->radio_state == WIFI_RADIO_ON && state->scanning) {
        lv_obj_clear_flag(view->scanning_spinner, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(view->scanning_spinner, LV_OBJ_FLAG_HIDDEN);
    }
}

static void update_wifi_toggle(WifiManageView* view, WifiManageState* state) {
    lv_obj_clear_state(view->enable_switch, LV_STATE_ANY);
    switch (state->radio_state) {
        case WIFI_RADIO_ON:
        case WIFI_RADIO_CONNECTION_PENDING:
        case WIFI_RADIO_CONNECTION_ACTIVE:
            lv_obj_add_state(view->enable_switch, LV_STATE_CHECKED);
            break;
        case WIFI_RADIO_ON_PENDING:
            lv_obj_add_state(view->enable_switch, LV_STATE_CHECKED | LV_STATE_DISABLED);
            break;
        case WIFI_RADIO_OFF:
            break;
        case WIFI_RADIO_OFF_PENDING:
            lv_obj_add_state(view->enable_switch, LV_STATE_DISABLED);
            break;
    }
}

static void update_connected_ap(WifiManageView* view, WifiManageState* state, TT_UNUSED WifiManageBindings* bindings) {
    switch (state->radio_state) {
        case WIFI_RADIO_CONNECTION_PENDING:
        case WIFI_RADIO_CONNECTION_ACTIVE:
            lv_obj_clear_flag(view->connected_ap_container, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(view->connected_ap_label, (const char*)state->connect_ssid);
            break;
        default:
            lv_obj_add_flag(view->connected_ap_container, LV_OBJ_FLAG_HIDDEN);
            break;
    }
}

// endregion Secondary updates

// region Main

void wifi_manage_view_create(App app, WifiManageView* view, WifiManageBindings* bindings, lv_obj_t* parent) {
    view->root = parent;

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    tt_toolbar_create_for_app(parent, app);

    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);

    // Top row: enable/disable
    lv_obj_t* switch_container = lv_obj_create(wrapper);
    lv_obj_set_width(switch_container, LV_PCT(100));
    lv_obj_set_height(switch_container, LV_SIZE_CONTENT);
    tt_lv_obj_set_style_no_padding(switch_container);
    tt_lv_obj_set_style_bg_invisible(switch_container);

    lv_obj_t* enable_label = lv_label_create(switch_container);
    lv_label_set_text(enable_label, "Wi-Fi");
    lv_obj_set_align(enable_label, LV_ALIGN_LEFT_MID);

    view->enable_switch = lv_switch_create(switch_container);
    lv_obj_add_event_cb(view->enable_switch, on_enable_switch_changed, LV_EVENT_ALL, bindings);
    lv_obj_set_align(view->enable_switch, LV_ALIGN_RIGHT_MID);

    view->connected_ap_container = lv_obj_create(wrapper);
    lv_obj_set_size(view->connected_ap_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(view->connected_ap_container, SPINNER_HEIGHT, 0);
    tt_lv_obj_set_style_no_padding(view->connected_ap_container);
    lv_obj_set_style_border_width(view->connected_ap_container, 0, 0);

    view->connected_ap_label = lv_label_create(view->connected_ap_container);
    lv_obj_align(view->connected_ap_label, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* disconnect_button = lv_btn_create(view->connected_ap_container);
    lv_obj_add_event_cb(disconnect_button, &on_disconnect_pressed, LV_EVENT_CLICKED, bindings);
    lv_obj_t* disconnect_label = lv_label_create(disconnect_button);
    lv_label_set_text(disconnect_label, "Disconnect");
    lv_obj_align(disconnect_button, LV_ALIGN_RIGHT_MID, 0, 0);

    // Networks

    lv_obj_t* networks_header = lv_obj_create(wrapper);
    lv_obj_set_size(networks_header, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(networks_header, SPINNER_HEIGHT, 0);
    tt_lv_obj_set_style_no_padding(networks_header);
    lv_obj_set_style_border_width(networks_header, 0, 0);

    view->networks_label = lv_label_create(networks_header);
    lv_label_set_text(view->networks_label, "Networks");
    lv_obj_align(view->networks_label, LV_ALIGN_LEFT_MID, 0, 0);

    view->scanning_spinner = lv_spinner_create(networks_header);
    lv_spinner_set_anim_params(view->scanning_spinner, 1000, 60);
    lv_obj_set_size(view->scanning_spinner, SPINNER_HEIGHT, SPINNER_HEIGHT);
    lv_obj_set_style_pad_top(view->scanning_spinner, 4, 0);
    lv_obj_set_style_pad_bottom(view->scanning_spinner, 4, 0);
    lv_obj_align_to(view->scanning_spinner, view->networks_label, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    view->networks_list = lv_obj_create(wrapper);
    lv_obj_set_flex_flow(view->networks_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(view->networks_list, LV_PCT(100));
    lv_obj_set_height(view->networks_list, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_top(view->networks_list, 8, 0);
    lv_obj_set_style_pad_bottom(view->networks_list, 8, 0);
}

void wifi_manage_view_update(WifiManageView* view, WifiManageBindings* bindings, WifiManageState* state) {
    update_wifi_toggle(view, state);
    update_scanning(view, state);
    update_network_list(view, state, bindings);
    update_connected_ap(view, state, bindings);
}
