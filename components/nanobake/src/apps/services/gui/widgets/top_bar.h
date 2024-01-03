#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TOP_BAR_ICON_SIZE 18
#define TOP_BAR_HEIGHT (TOP_BAR_ICON_SIZE + 4) // 4 extra pixels for border and outline

void top_bar(lv_obj_t* parent);

#ifdef __cplusplus
}
#endif
