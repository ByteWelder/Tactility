#include "Display.h"
#include "Constants.h"
#include <Ssd1306Display.h>

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {

    auto configuration = std::make_unique<Ssd1306Display::Configuration>(
        HELTEC_LCD_I2C_PORT,
        HELTEC_LCD_I2C_ADDRESS,
        HELTEC_LCD_PIN_RST,
        HELTEC_LCD_HORIZONTAL_RESOLUTION,
        HELTEC_LCD_VERTICAL_RESOLUTION,
        nullptr, // no touch
        false // invert
    );

    configuration->gapX = -4;
    
    auto display = std::make_shared<Ssd1306Display>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
