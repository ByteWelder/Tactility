#include "Core2Display.h"
#include "Core2Touch.h"

#include <Ili934xDisplay.h>

std::shared_ptr<tt::hal::Display> createDisplay() {
    auto touch = std::make_shared<Core2Touch>();

    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        CORE2_LCD_SPI_HOST,
        CORE2_LCD_PIN_CS,
        CORE2_LCD_PIN_DC,
        CORE2_LCD_HORIZONTAL_RESOLUTION,
        CORE2_LCD_VERTICAL_RESOLUTION,
        touch
    );

    return std::make_shared<Ili934xDisplay>(std::move(configuration));
}
