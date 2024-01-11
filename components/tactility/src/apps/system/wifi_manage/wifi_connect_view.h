#pragma once

#include "lvgl.h"
#include "wifi_state.h"
#include "wifi_view.h"
#include "wifi_bindings.h"

#ifdef __cplusplus
extern "C" {
#endif

void wifi_connect_view_create(WifiView* view, WifiBindings* bindings, lv_obj_t* parent);
void wifi_connect_view_update(WifiView* view, WifiState* state);

#ifdef __cplusplus
}
#endif
