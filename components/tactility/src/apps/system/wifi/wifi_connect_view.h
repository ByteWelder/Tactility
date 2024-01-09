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

void wifi_connect_view_create(WifiConnectView* view, lv_obj_t* parent);
void wifi_connect_view_update(WifiConnectView* view, WifiState* state);
void wifi_connect_view_clear(WifiConnectView* view);

#ifdef __cplusplus
}
#endif
