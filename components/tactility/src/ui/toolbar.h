#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*NavAction)();

typedef struct {
    const char* _Nullable title;
    const char* _Nullable nav_icon; // LVGL compatible definition (e.g. local file or embedded icon from LVGL)
    NavAction nav_action;
} Toolbar;

lv_obj_t* tt_lv_toolbar_create(lv_obj_t* parent, const Toolbar* toolbar);

#ifdef __cplusplus
}
#endif
