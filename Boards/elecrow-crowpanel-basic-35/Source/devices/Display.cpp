#include "Display.h"

#include <Ili9488Display.h>
#include <PwmBacklight.h>
#include <Xpt2046Touch.h>

std::shared_ptr<Xpt2046Touch> createTouch() {
    auto configuration = std::make_unique<Xpt2046Touch::Configuration>(
        CROWPANEL_LCD_SPI_HOST,
        CROWPANEL_TOUCH_PIN_CS,
        320,
        480,
        false,
        false,
        true
    );

    return std::make_shared<Xpt2046Touch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();

    auto configuration = std::make_unique<Ili9488Display::Configuration>(
        CROWPANEL_LCD_SPI_HOST,
        CROWPANEL_LCD_PIN_CS,
        CROWPANEL_LCD_PIN_DC,
        320,
        480,
        touch,
        false,
        false,
        false
    );

    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    auto display = std::make_shared<Ili9488Display>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
