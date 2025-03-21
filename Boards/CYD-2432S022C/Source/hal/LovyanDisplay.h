#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <memory>
#include "lvgl.h"

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
