#pragma once

#include "lvgl.h"

void keyboard_wait_for_response();

typedef void* Keyboard;

Keyboard keyboard_alloc(_Nullable lv_disp_t* display);
void keyboard_free(Keyboard keyboard);