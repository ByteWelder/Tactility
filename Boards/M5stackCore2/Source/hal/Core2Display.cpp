#include "Core2Display.h"

#include <Ft6x36Touch.h>
#include <Ili934xDisplay.h>

std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    auto configuration = std::make_unique<Ft6x36Touch::Configuration>(
        I2C_NUM_0,
        GPIO_NUM_39
    );

    return std::make_shared<Ft6x36Touch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();

    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        CORE2_LCD_SPI_HOST,
        CORE2_LCD_PIN_CS,
        CORE2_LCD_PIN_DC,
        CORE2_LCD_HORIZONTAL_RESOLUTION,
        CORE2_LCD_VERTICAL_RESOLUTION,
        touch,
        false,
        false,
        false,
        true
    );

    return std::make_shared<Ili934xDisplay>(std::move(configuration));
}
