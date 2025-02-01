#pragma once

#include "tt_app.h"
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Create a toolbar widget that shows the app name as title */
lv_obj_t* tt_lvgl_toolbar_create(lv_obj_t* parent, AppHandle context);

/** Create a toolbar widget with the provided title*/
lv_obj_t* tt_lvgl_toolbar_create_simple(lv_obj_t* parent, const char* title);

#ifdef __cplusplus
}
#endif