#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void tt_lv_label_set_text_file(lv_obj_t* label, const char* filepath);

#ifdef __cplusplus
}
#endif