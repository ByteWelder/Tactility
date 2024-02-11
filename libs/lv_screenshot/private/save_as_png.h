#pragma once

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//#if LV_USE_PNG == 0
//  #error "LV_USE_PNG must be defined in lv_conf.h"
//#endif

bool save_as_png_file(const uint8_t* image, uint32_t w, uint32_t h, uint32_t bpp, const char* filename);

#ifdef __cplusplus
}
#endif
