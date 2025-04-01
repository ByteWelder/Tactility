#pragma once

#include <src/display/lv_display.h>

namespace tt::app::display {

void setBacklightDuty(uint8_t value);

bool getBacklightDuty(uint8_t& duty);

void setGammaCurve(uint8_t curveIndex);

bool getGammaCurve(uint8_t& curveIndex);

void setRotation(lv_display_rotation_t rotation);

lv_display_rotation_t getRotation();

} // namespace
