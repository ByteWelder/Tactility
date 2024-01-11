#pragma once

#include "lvgl.h"
#include "wifi_manage_bindings.h"
#include "wifi_manage_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* root;
    lv_obj_t* enable_switch;
    lv_obj_t* scanning_spinner;
    lv_obj_t* networks_label;
    lv_obj_t* networks_list;
} WifiManageView;

void wifi_manage_view_create(WifiManageView* view, WifiManageBindings* bindings, lv_obj_t* parent);
void wifi_manage_view_update(WifiManageView* view, WifiManageBindings* bindings, WifiManageState* state);

#ifdef __cplusplus
}
#endif
