#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <memory>
#include "esp_err.h"
#include "lvgl.h"

namespace tt::hal::display {

std::shared_ptr<DisplayDevice> createDisplay();
void setRotation(lv_display_rotation_t rotation);
esp_lcd_panel_handle_t getLcdPanelHandle();

} // namespace tt::hal::display
