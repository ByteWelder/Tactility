#include "YellowDisplay.h"
#include "Xpt2046Touch.h"
#include "YellowConstants.h"
#include <Ili934xDisplay.h>
#include <PwmBacklight.h>

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    auto configuration = std::make_unique<Xpt2046Touch::Configuration>(
        CYD2432S028R_TOUCH_SPI_HOST,
        CYD2432S028R_TOUCH_PIN_CS,
        240,
        320,
        false,
        true,
        false
    );

    return std::make_shared<Xpt2046Touch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();

    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        CYD2432S028R_LCD_SPI_HOST,
        CYD2432S028R_LCD_PIN_CS,
        CYD2432S028R_LCD_PIN_DC,
        CYD2432S028R_LCD_HORIZONTAL_RESOLUTION,
        CYD2432S028R_LCD_VERTICAL_RESOLUTION,
        touch,
        false,
        true,
        false,
        false,
        CYD2432S028R_LCD_DRAW_BUFFER_SIZE
    );

    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    auto display = std::make_shared<Ili934xDisplay>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
