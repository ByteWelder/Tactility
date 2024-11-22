#pragma once

#include <cstdint>
#include <src/display/lv_display.h>

namespace tt::app::settings::display {

void preferences_set_backlight_duty(uint8_t value);
uint8_t preferences_get_backlight_duty();
void preferences_set_rotation(lv_display_rotation_t rotation);
lv_display_rotation_t preferences_get_rotation();

} // namespace
