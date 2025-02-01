#pragma once

#include <TactilityCore.h>
#include <Mutex.h>
#include <Thread.h>
#include "lvgl.h"
#include <Tactility/hal/i2c/I2c.h>
#include "Timer.h"
#include <memory>

namespace tt::app::i2cscanner {

void start();

}
