#pragma once

#include "lvgl.h"
#include "wifi_connect_state.h"
#include "wifi_connect_bindings.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* root;
    lv_obj_t* ssid_textarea;
    lv_obj_t* password_textarea;
    lv_obj_t* connect_button;
    lv_obj_t* cancel_button;
} WifiConnectView;

void wifi_connect_view_create(App app, void* wifi, lv_obj_t* parent);
void wifi_connect_view_update(WifiConnectView* view, WifiConnectBindings* bindings, WifiConnectState* state);

#ifdef __cplusplus
}
#endif
