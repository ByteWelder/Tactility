#include "Display.h"

#include <Ft5x06Touch.h>
#include <PwmBacklight.h>
#include <St7789Display.h>

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    // Note for future changes: Reset pin is 48 and interrupt pin is 47
    auto configuration = std::make_unique<Ft5x06Touch::Configuration>(
        I2C_NUM_0,
        240,
        320,
        false,
        false,
        false
    );

    return std::make_shared<Ft5x06Touch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();

    auto configuration = std::make_unique<St7789Display::Configuration>(
        CROWPANEL_LCD_SPI_HOST,
        CROWPANEL_LCD_PIN_CS,
        CROWPANEL_LCD_PIN_DC,
        240,
        320,
        touch,
        false,
        false,
        false,
        true
    );

    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    auto display = std::make_shared<St7789Display>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
