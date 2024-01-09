#include "wifi_connect_view.h"

#include "lvgl.h"
#include "wifi_state.h"

#define TAG "wifi_connect_view"

void wifi_connect_view_create(WifiView* wifi_view, lv_obj_t* parent) {
    WifiConnectView* connect_view = &wifi_view->connect_view;
    connect_view->root = parent;
}

void wifi_connect_view_update(WifiView* view, WifiState* state) {
}
