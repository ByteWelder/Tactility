#pragma once

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool lv_screenshot_save_png_file(const uint8_t* image, uint32_t w, uint32_t h, uint32_t bpp, const char* filename);

#ifdef __cplusplus
}
#endif
