#include "YellowTouch.h"
#include "CYD2432S022CConstants.h"
#include "Cst816Touch.h"

std::shared_ptr<tt::hal::touch::TouchDevice> createYellowTouch() {
    auto configuration = std::make_unique<Cst816sTouch::Configuration>(
        CYD_2432S022C_TOUCH_I2C_PORT,           // I2C_NUM_0
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION, // 240
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION    // 320
        // Optional: Add mirroring or swapping if needed (see below)
    );
    return std::make_shared<Cst816sTouch>(std::move(configuration));
}
