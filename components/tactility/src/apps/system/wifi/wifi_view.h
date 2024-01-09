#pragma once

#include "lvgl.h"
#include "wifi_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* root;
    lv_obj_t* textarea;
    lv_obj_t* ok_button;
    lv_obj_t* cancel_button;
} WifiConnectView;

typedef struct {
    lv_obj_t* root;
    lv_obj_t* enable_switch;
    lv_obj_t* scanning_spinner;
    lv_obj_t* networks_label;
    lv_obj_t* networks_list;
} WifiMainView;

typedef struct {
    lv_obj_t* root;
    WifiMainView main_view;
    WifiConnectView connect_view;
} WifiView;

void wifi_view_create(WifiView* view, lv_obj_t* parent);
void wifi_view_update(WifiView* view, WifiState* state);

#ifdef __cplusplus
}
#endif
