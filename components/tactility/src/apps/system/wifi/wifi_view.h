#pragma once

#include "lvgl.h"
#include "wifi_state.h"
#include "wifi_main_view.h"
#include "wifi_connect_view.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_VIEW_MAIN,
    WIFI_VIEW_CONNECT
} ActiveView;

typedef struct {
    lv_obj_t* root;
    ActiveView active_view;
    WifiMainView main_view;
    WifiConnectView connect_view;
} WifiView;

void wifi_view_create(WifiView* view, lv_obj_t* parent);
void wifi_view_update(WifiView* view, WifiState* state);
void wifi_view_clear(WifiView* view);

#ifdef __cplusplus
}
#endif
