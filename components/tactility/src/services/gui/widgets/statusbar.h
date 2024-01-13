#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STATUSBAR_ICON_SIZE 18
#define STATUSBAR_HEIGHT (STATUSBAR_ICON_SIZE + 4) // 4 extra pixels for border and outline

lv_obj_t* tt_lv_statusbar_create(lv_obj_t* parent);

#ifdef __cplusplus
}
#endif
