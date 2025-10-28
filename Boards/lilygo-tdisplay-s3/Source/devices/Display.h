#pragma once

#include <Tactility/hal/display/DisplayDevice.h>

class St7789i8080Display;

// Factory function for registration
std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
