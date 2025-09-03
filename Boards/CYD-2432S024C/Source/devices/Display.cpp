#include "Display.h"
#include "Cst816Touch.h"

#include <Ili934xDisplay.h>
#include <PwmBacklight.h>

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    // Note: GPIO 25 for reset and GPIO 21 for interrupt?
    auto configuration = std::make_unique<Cst816sTouch::Configuration>(
        I2C_NUM_0,
        240,
        320
    );

    return std::make_shared<Cst816sTouch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {

    auto touch = createTouch();

    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        TWODOTFOUR_LCD_SPI_HOST,
        TWODOTFOUR_LCD_PIN_CS,
        TWODOTFOUR_LCD_PIN_DC,
        TWODOTFOUR_LCD_HORIZONTAL_RESOLUTION,
        TWODOTFOUR_LCD_VERTICAL_RESOLUTION,
        touch
    );

    configuration->mirrorX = true;
    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    auto display = std::make_shared<Ili934xDisplay>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
