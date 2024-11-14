#pragma once

#include "lvgl.h"
#include "wifi_connect_state.h"
#include "wifi_connect_bindings.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* ssid_textarea;
    lv_obj_t* ssid_error;
    lv_obj_t* password_textarea;
    lv_obj_t* password_error;
    lv_obj_t* connect_button;
    lv_obj_t* cancel_button;
    lv_obj_t* remember_switch;
    lv_obj_t* connecting_spinner;
    lv_obj_t* connection_error;
    lv_group_t* group;
} WifiConnectView;

void wifi_connect_view_create(App app, void* wifi, lv_obj_t* parent);
void wifi_connect_view_update(WifiConnectView* view, WifiConnectBindings* bindings, WifiConnectState* state);
void wifi_connect_view_destroy(WifiConnectView* view);

#ifdef __cplusplus
}
#endif
