#pragma once

#include "hal/Power.h"
#include "hal/Sdcard.h"

extern bool m5stack_bootstrap();
extern bool m5stack_lvgl_init();

extern const tt::hal::Power m5stack_power;
extern const tt::hal::sdcard::SdCard m5stack_sdcard;
