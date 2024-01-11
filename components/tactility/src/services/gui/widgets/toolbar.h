#pragma once

#include "lvgl.h"
#include "app_manifest.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TOOLBAR_HEIGHT 40
#define TOOLBAR_FONT_HEIGHT 18

lv_obj_t* tt_lv_toolbar_create(lv_obj_t* parent, lv_coord_t offset_y, const AppManifest* manifest);

#ifdef __cplusplus
}
#endif
