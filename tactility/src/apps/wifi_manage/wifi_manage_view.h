#pragma once

#include "app.h"
#include "lvgl.h"
#include "wifi_manage_bindings.h"
#include "wifi_manage_state.h"

typedef struct {
    lv_obj_t* root;
    lv_obj_t* enable_switch;
    lv_obj_t* scanning_spinner;
    lv_obj_t* networks_label;
    lv_obj_t* networks_list;
    lv_obj_t* connected_ap_container;
    lv_obj_t* connected_ap_label;
} WifiManageView;

void wifi_manage_view_create(App app, WifiManageView* view, WifiManageBindings* bindings, lv_obj_t* parent);
void wifi_manage_view_update(WifiManageView* view, WifiManageBindings* bindings, WifiManageState* state);
