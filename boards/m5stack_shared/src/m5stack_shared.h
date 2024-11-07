#pragma once

#include "power.h"
#include "sdcard.h"

#ifdef __cplusplus
extern "C" {
#endif

extern bool m5stack_lvgl_init();
extern bool m5stack_bootstrap();

extern Power m5stack_power;
extern const SdCard m5stack_sdcard;

#ifdef __cplusplus
}
#endif