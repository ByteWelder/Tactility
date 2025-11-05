#include "Display.h"
#include "Constants.h"

#include <Ssd1306Display.h>

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto configuration = std::make_unique<Ssd1306Display::Configuration>(
        DISPLAY_I2C_PORT,
        DISPLAY_I2C_ADDRESS,
        DISPLAY_PIN_RST,
        DISPLAY_HORIZONTAL_RESOLUTION,
        DISPLAY_VERTICAL_RESOLUTION,
        nullptr, // no touch
        false // invert
    );

    auto display = std::make_shared<Ssd1306Display>(std::move(configuration));
    return std::static_pointer_cast<tt::hal::display::DisplayDevice>(display);
}