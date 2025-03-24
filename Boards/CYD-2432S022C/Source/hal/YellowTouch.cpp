#include "YellowTouch.h"
#include "CYD2432S022CConstants.h"
#include "Cst816Touch.h" // From Drivers/CST816S/
#include "Tactility/app/display/DisplaySettings.h"
#include <memory>

std::shared_ptr<tt::hal::touch::TouchDevice> createYellowTouch() {
    lv_display_rotation_t rotation = tt::app::display::getRotation();
    bool swapXY = (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270);
    bool mirrorX = (rotation == LV_DISPLAY_ROTATION_270);
    bool mirrorY = (rotation == LV_DISPLAY_ROTATION_90);
    auto configuration = std::make_unique<Cst816sTouch::Configuration>(
        CYD_2432S022C_TOUCH_I2C_PORT,
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION,
        swapXY,
        mirrorX,
        mirrorY
    );
    return std::make_shared<Cst816sTouch>(std::move(configuration));
}
