#pragma once

#include "App.h"
#include "WifiManageBindings.h"
#include "WifiManageState.h"
#include "lvgl.h"

namespace tt::app::wifi_manage {

typedef struct {
    lv_obj_t* root;
    lv_obj_t* enable_switch;
    lv_obj_t* scanning_spinner;
    lv_obj_t* networks_label;
    lv_obj_t* networks_list;
    lv_obj_t* connected_ap_container;
    lv_obj_t* connected_ap_label;
} WifiManageView;

void view_create(App app, WifiManageView* view, WifiManageBindings* bindings, lv_obj_t* parent);
void view_update(WifiManageView* view, WifiManageBindings* bindings, WifiManageState* state);

} // namespace
