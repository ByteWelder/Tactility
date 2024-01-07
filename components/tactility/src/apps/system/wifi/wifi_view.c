#include "wifi_view.h"
#include "services/wifi/wifi.h"

static void on_enable_switch_changed(lv_event_t* event) {
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t* enable_switch = lv_event_get_target(event);
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool is_on = lv_obj_has_state(enable_switch, LV_STATE_CHECKED);
        wifi_set_enabled(is_on);
    }
}

void wifi_view_create(WifiView* view, lv_obj_t* parent) {
    view->enable_switch = lv_switch_create(parent);
    lv_obj_add_event_cb(view->enable_switch, on_enable_switch_changed, LV_EVENT_ALL, NULL);

    view->scanning_spinner = lv_spinner_create(parent, 1000, 60);
    lv_obj_set_size(view->scanning_spinner, 32, 32);
    lv_obj_center(view->scanning_spinner);
}

void wifi_view_update(WifiView* view, WifiState* state) {
    if (state->radio_state == WIFI_RADIO_ON && state->scanning) {
        lv_obj_clear_flag(view->scanning_spinner, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(view->scanning_spinner, LV_OBJ_FLAG_HIDDEN);
    }

    switch (state->radio_state) {
        case WIFI_RADIO_ON:
            lv_obj_add_state(view->enable_switch, LV_STATE_CHECKED);
            lv_obj_clear_state(view->enable_switch, LV_STATE_DISABLED);
            break;
        case WIFI_RADIO_ON_PENDING:
            lv_obj_add_state(view->enable_switch, LV_STATE_CHECKED);
            lv_obj_add_state(view->enable_switch, LV_STATE_DISABLED);
            break;
        case WIFI_RADIO_OFF:
            lv_obj_clear_state(view->enable_switch, LV_STATE_CHECKED);
            lv_obj_clear_state(view->enable_switch, LV_STATE_DISABLED);
            break;
        case WIFI_RADIO_OFF_PENDING:
            lv_obj_clear_state(view->enable_switch, LV_STATE_CHECKED);
            lv_obj_add_state(view->enable_switch, LV_STATE_DISABLED);
            break;
    }
}

void wifi_view_clear(WifiView* view) {
    view->scanning_spinner = NULL;
    view->enable_switch = NULL;
}
