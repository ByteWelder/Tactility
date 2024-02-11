#pragma once

#include "lvgl.h"
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LV_100ASK_SCREENSHOT_SV_BMP = 0,
    LV_100ASK_SCREENSHOT_SV_PNG = 1,
    LV_100ASK_SCREENSHOT_SV_LAST
} lv_100ask_screenshot_sv_t;

bool lv_screenshot_create(lv_obj_t* obj, lv_img_cf_t cf, lv_100ask_screenshot_sv_t screenshot_sv, const char* filename);

#ifdef __cplusplus
} /*extern "C"*/
#endif
