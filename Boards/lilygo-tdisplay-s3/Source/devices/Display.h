#pragma once

#include <Tactility/hal/display/DisplayDevice.h>

class I8080St7789Display;

// Factory function for registration
std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
