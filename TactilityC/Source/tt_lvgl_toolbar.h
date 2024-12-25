#pragma once

#include <lvgl.h>
#include "tt_app_context.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* tt_lvgl_toolbar_create(lv_obj_t* parent, AppContextHandle context);
lv_obj_t* tt_lvgl_toolbar_create_simple(lv_obj_t* parent, const char* title);

#ifdef __cplusplus
}
#endif