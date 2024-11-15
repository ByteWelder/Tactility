#pragma once

#include "lvgl.h"
#include <cstdint>

bool ws3t_display_lock(uint32_t timeout_ms);
void ws3t_display_unlock(void);

lv_display_t* ws3t_display_create();
void ws3t_display_destroy();