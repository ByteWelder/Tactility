#pragma once

#include "Hal/Power.h"
#include "Hal/Sdcard.h"

extern bool m5stack_bootstrap();
extern bool m5stack_lvgl_init();

extern const tt::hal::Power m5stack_power;
extern const tt::hal::sdcard::SdCard m5stack_sdcard;
