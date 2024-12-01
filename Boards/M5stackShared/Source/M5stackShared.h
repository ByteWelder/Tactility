#pragma once

#include "hal/Power.h"
#include "hal/M5stackTouch.h"
#include "hal/M5stackDisplay.h"
#include "hal/sdcard/Sdcard.h"

extern bool m5stack_bootstrap();
extern bool m5stack_lvgl_init();

extern const tt::hal::Power m5stack_power;
extern const tt::hal::sdcard::SdCard m5stack_sdcard;
