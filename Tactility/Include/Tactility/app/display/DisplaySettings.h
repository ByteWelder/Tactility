#pragma once

#include <src/display/lv_display.h>

namespace tt::app::display {

void setBacklightDuty(uint8_t value);
uint8_t getBacklightDuty();
void setRotation(lv_display_rotation_t rotation);
lv_display_rotation_t getRotation();

} // namespace
