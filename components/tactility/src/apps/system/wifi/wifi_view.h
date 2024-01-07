#pragma once

#include "lvgl.h"
#include "wifi_state.h"

typedef struct {
    lv_obj_t* enable_switch;
    lv_obj_t* scanning_spinner;
    lv_obj_t* networks_label;
    lv_obj_t* networks_list;
} WifiView;

void wifi_view_create(WifiView* view, lv_obj_t* parent);
void wifi_view_update(WifiView* view, WifiState* state);
void wifi_view_clear(WifiView* view);
