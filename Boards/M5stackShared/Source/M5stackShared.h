#pragma once

#include "hal/Power.h"
#include "hal/M5stackTouch.h"
#include "hal/M5stackDisplay.h"
#include "hal/M5stackPower.h"
#include "hal/M5stackSdCard.h"

extern bool m5stack_bootstrap();
extern bool m5stack_lvgl_init();
