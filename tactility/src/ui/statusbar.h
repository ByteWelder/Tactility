#pragma once

#include "lvgl.h"
#include "app.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STATUSBAR_ICON_LIMIT 8
#define STATUSBAR_ICON_SIZE 20
#define STATUSBAR_HEIGHT (STATUSBAR_ICON_SIZE + 4) // 4 extra pixels for border and outline

lv_obj_t* tt_statusbar_create(lv_obj_t* parent);
int8_t tt_statusbar_icon_add(const char* image);
void tt_statusbar_icon_remove(int8_t id);
void tt_statusbar_icon_set_image(int8_t id, const char* image);
void tt_statusbar_icon_set_visibility(int8_t id, bool visible);

#ifdef __cplusplus
}
#endif
