#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include "lvgl.h"
#include <memory>

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
