#include "YellowDisplay.h"
#include "YellowDisplayConstants.h"

#include <Ili934xDisplay.h>
#include <Xpt2046Touch.h>

#include <PwmBacklight.h>

std::shared_ptr<Xpt2046Touch> createTouch() {
    auto configuration = std::make_unique<Xpt2046Touch::Configuration>(
        SPI1_HOST,
        YELLOW_TOUCH_PIN_CS,
        YELLOW_LCD_HORIZONTAL_RESOLUTION,  // 240
        YELLOW_LCD_VERTICAL_RESOLUTION,    // 320
        false,  // swapXY
        true,   // mirrorX
        false   // mirrorY
    );

    return std::make_shared<Xpt2046Touch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();

    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        YELLOW_LCD_SPI_HOST,
        YELLOW_LCD_PIN_CS,
        YELLOW_LCD_PIN_DC,
        YELLOW_LCD_HORIZONTAL_RESOLUTION,  // 240
        YELLOW_LCD_VERTICAL_RESOLUTION,    // 320
        touch
    );

    configuration->mirrorX = true;
    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    return std::make_shared<Ili934xDisplay>(std::move(configuration));
}
