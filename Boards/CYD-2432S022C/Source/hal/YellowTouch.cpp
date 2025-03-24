#include "YellowTouch.h"
#include "CYD2432S022CConstants.h"
#include "Cst816Touch.h"
#include "Tactility/app/display/DisplaySettings.h"

std::shared_ptr<tt::hal::touch::TouchDevice> createYellowTouch() {
    lv_display_rotation_t rotation = tt::app::display::getRotation();
    bool mirrorX = (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270);
    bool mirrorY = (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270);
    auto configuration = std::make_unique<Cst816sTouch::Configuration>(
        CYD_2432S022C_TOUCH_I2C_PORT,           // I2C_NUM_0
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION, // 240
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION,   // 320
        false,   // swapXY
        mirrorX, // Mirror X in landscape
        mirrorY  // Mirror Y in landscape
    );
    return std::make_shared<Cst816sTouch>(std::move(configuration));
}
