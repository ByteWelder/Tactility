#pragma once

#include "lvgl.h"
#include "wifi_state.h"

typedef struct {
    lv_obj_t* spinner;
} WifiView;

void wifi_view_create(WifiView* view, lv_obj_t* parent);
void wifi_view_update(WifiView* view, WifiState* state);
void wifi_view_clear(WifiView* view);
