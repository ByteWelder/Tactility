#include "YellowTouch.h"
#include "CYD2432S022CConstants.h"
#include "Cst816Touch.h"
#include "Tactility/app/display/DisplaySettings.h"

std::shared_ptr<tt::hal::touch::TouchDevice> createYellowTouch() {
    lv_display_rotation_t rotation = tt::app::display::getRotation();
    bool swapXY = (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270);
    bool mirrorX = (rotation == LV_DISPLAY_ROTATION_270); // Mirror X when USB is left
    bool mirrorY = (rotation == LV_DISPLAY_ROTATION_90);  // Mirror Y when USB is right
    auto configuration = std::make_unique<Cst816sTouch::Configuration>(
        CYD_2432S022C_TOUCH_I2C_PORT,
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION, // 240
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION,   // 320
        swapXY,   // Swap X/Y in landscape
        mirrorX,  // Mirror X as needed
        mirrorY   // Mirror Y as needed
    );
    return std::make_shared<Cst816sTouch>(std::move(configuration));
}
