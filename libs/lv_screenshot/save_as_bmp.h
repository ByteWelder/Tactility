#pragma once

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool save_as_bmp_file(const uint8_t* image, uint32_t w, uint32_t h, uint32_t bpp, const char* filename);

#ifdef __cplusplus
}
#endif
