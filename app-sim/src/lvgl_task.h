#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void lvgl_task_start();
bool lvgl_task_is_running();
void lvgl_task_interrupt();

#ifdef __cplusplus
}
#endif