#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool lvgl_is_ready();
void lvgl_interrupt();

#ifdef __cplusplus
}
#endif