#pragma once

#include "lvgl.h"
#include "wifi_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* root;
    lv_obj_t* enable_switch;
    lv_obj_t* scanning_spinner;
    lv_obj_t* networks_label;
    lv_obj_t* networks_list;
} WifiMainView;

void wifi_main_view_create(WifiMainView* view, lv_obj_t* parent);
void wifi_main_view_update(WifiMainView* view, WifiState* state);
void wifi_main_view_clear(WifiMainView* view);

#ifdef __cplusplus
}
#endif
