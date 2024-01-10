#pragma once

#include "lvgl.h"
#include "wifi_bindings.h"
#include "wifi_state.h"
#include "wifi_view.h"

#ifdef __cplusplus
extern "C" {
#endif

void wifi_main_view_create(WifiView* wifi_view, WifiBindings* bindings, lv_obj_t* parent);
void wifi_main_view_update(WifiView* view, WifiState* state, WifiBindings* bindings);

#ifdef __cplusplus
}
#endif
