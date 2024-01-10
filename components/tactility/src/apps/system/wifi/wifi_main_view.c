#include "wifi_main_view.h"

#include "services/gui/widgets/widgets.h"
#include "services/wifi/wifi.h"
#include "log.h"
#include "wifi_state.h"
#include "wifi_view.h"

#define TAG "wifi_main_view"
#define SPINNER_HEIGHT 40

static void on_enable_switch_changed(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t* enable_switch = lv_event_get_target(event);
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool is_on = lv_obj_has_state(enable_switch, LV_STATE_CHECKED);
        WifiBindings* bindings = (WifiBindings*)event->user_data;
        bindings->on_wifi_toggled(is_on);
    }
}

// region Secondary updates

static const char* get_network_icon(int8_t rssi, wifi_auth_mode_t auth_mode) {
    if (rssi > -67) {
        if (auth_mode == WIFI_AUTH_OPEN)
            return "A:/assets/network_wifi.png";
        else
            return "A:/assets/network_wifi_locked.png";
    } else if (rssi > -70) {
        if (auth_mode == WIFI_AUTH_OPEN)
            return "A:/assets/network_wifi_3_bar.png";
        else
            return "A:/assets/network_wifi_3_bar_locked.png";
    } else if (rssi > -80) {
        if (auth_mode == WIFI_AUTH_OPEN)
            return "A:/assets/network_wifi_2_bar.png";
        else
            return "A:/assets/network_wifi_2_bar_locked.png";
    } else {
        if (auth_mode == WIFI_AUTH_OPEN)
            return "A:/assets/network_wifi_1_bar.png";
        else
            return "A:/assets/network_wifi_1_bar_locked.png";
    }
}

static void connect(lv_event_t* event) {
    lv_obj_t* button = event->current_target;
    // Assumes that the second child of the button is a label ... risky
    lv_obj_t* label = lv_obj_get_child(button, 1);
    // We get the SSID from the button label because it's safer than alloc'ing
    // our own and passing it as the event data
    const char* ssid = lv_label_get_text(label);
    FURI_LOG_I(TAG, "Clicked AP: %s", ssid);
    WifiBindings* bindings = (WifiBindings*)event->user_data;
    bindings->on_connect_ssid(ssid, bindings->on_connect_ssid_context);
}

static void create_network_button(WifiMainView* view, WifiBindings* bindings, WifiApRecord* record) {
    const char* ssid = (const char*)record->ssid;
    const char* icon = get_network_icon(record->rssi, record->auth_mode);
    lv_obj_t* ap_button = lv_list_add_btn(
        view->networks_list,
        icon,
        ssid
    );
    lv_obj_add_event_cb(ap_button, &connect, LV_EVENT_CLICKED, bindings);
}

static void update_network_list(WifiMainView* view, WifiState* state, WifiBindings* bindings) {
    lv_obj_clean(view->networks_list);

    if (state->radio_state == WIFI_RADIO_ON) {
        lv_obj_clear_flag(view->networks_label, LV_OBJ_FLAG_HIDDEN);

        WifiApRecord records[16];
        uint16_t count = 0;
        wifi_get_scan_results(records, 16, &count);
        if (count > 0) {
            for (int i = 0; i < count; ++i) {
                create_network_button(view, bindings, &records[i]);
            }
            lv_obj_clear_flag(view->networks_list, LV_OBJ_FLAG_HIDDEN);
        } else if (state->scanning) {
            lv_obj_add_flag(view->networks_list, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(view->networks_list, LV_OBJ_FLAG_HIDDEN);
            lv_obj_t* label = lv_label_create(view->networks_list);
            lv_label_set_text(label, "No networks found.");
        }
    } else {
        lv_obj_add_flag(view->networks_list, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(view->networks_label, LV_OBJ_FLAG_HIDDEN);
    }
}

void update_scanning(WifiMainView* view, WifiState* state) {
    if (state->radio_state == WIFI_RADIO_ON && state->scanning) {
        lv_obj_clear_flag(view->scanning_spinner, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(view->scanning_spinner, LV_OBJ_FLAG_HIDDEN);
    }
}

static void update_wifi_toggle(WifiMainView* view, WifiState* state) {
    lv_obj_clear_state(view->enable_switch, LV_STATE_ANY);
    switch (state->radio_state) {
        case WIFI_RADIO_ON:
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

// endregion Secondary updates

// region Main

void wifi_main_view_create(WifiView* wifi_view, WifiBindings* bindings, lv_obj_t* parent) {
    WifiMainView* main_view = &wifi_view->main_view;
    main_view->root = parent;

    // TODO: Standardize this into "window content" function?
    // TODO: It can then be dynamically determined based on screen res and size
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_top(parent, 8, 0);
    lv_obj_set_style_pad_bottom(parent, 8, 0);
    lv_obj_set_style_pad_left(parent, 16, 0);
    lv_obj_set_style_pad_right(parent, 16, 0);

    // Top row: enable/disable
    lv_obj_t* switch_container = lv_obj_create(parent);
    lv_obj_set_width(switch_container, LV_PCT(100));
    lv_obj_set_height(switch_container, LV_SIZE_CONTENT);
    lv_obj_set_style_no_padding(switch_container);
    lv_obj_set_style_bg_invisible(switch_container);

    lv_obj_t* enable_label = lv_label_create(switch_container);
    lv_label_set_text(enable_label, "Wi-Fi");
    lv_obj_set_align(enable_label, LV_ALIGN_LEFT_MID);

    main_view->enable_switch = lv_switch_create(switch_container);
    lv_obj_add_event_cb(main_view->enable_switch, on_enable_switch_changed, LV_EVENT_ALL, bindings);
    lv_obj_set_align(main_view->enable_switch, LV_ALIGN_RIGHT_MID);

    // Networks

    main_view->networks_label = lv_label_create(parent);
    lv_label_set_text(main_view->networks_label, "Networks");
    lv_obj_set_style_text_align(main_view->networks_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(main_view->networks_label, 8, 0);
    lv_obj_set_style_pad_bottom(main_view->networks_label, 8, 0);
    lv_obj_set_style_pad_left(main_view->networks_label, 2, 0);
    lv_obj_set_align(main_view->networks_label, LV_ALIGN_LEFT_MID);

    main_view->scanning_spinner = lv_spinner_create(parent, 1000, 60);
    lv_obj_set_size(main_view->scanning_spinner, SPINNER_HEIGHT, SPINNER_HEIGHT);
    lv_obj_set_style_pad_top(main_view->scanning_spinner, 4, 0);
    lv_obj_set_style_pad_bottom(main_view->scanning_spinner, 4, 0);

    main_view->networks_list = lv_obj_create(parent);
    lv_obj_set_flex_flow(main_view->networks_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(main_view->networks_list, LV_PCT(100));
    lv_obj_set_height(main_view->networks_list, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_top(main_view->networks_list, 8, 0);
    lv_obj_set_style_pad_bottom(main_view->networks_list, 8, 0);
}

void wifi_main_view_update(WifiView* view, WifiState* state, WifiBindings* bindings) {
    update_wifi_toggle(&view->main_view, state);
    update_scanning(&view->main_view, state);
    update_network_list(&view->main_view, state, bindings);
}
