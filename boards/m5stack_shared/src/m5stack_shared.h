#pragma once

#include "power.h"
#include "sdcard.h"

#ifdef __cplusplus
extern "C" {
#endif

extern bool m5stack_lvgl_init();
extern bool m5stack_bootstrap();

extern Power m5stack_power;

#ifdef __cplusplus
}
#endif