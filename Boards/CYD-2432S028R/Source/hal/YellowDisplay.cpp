#include "YellowDisplay.h"
#include "YellowDisplayConstants.h"

#include <Ili934xDisplay.h>
#include <Xpt2046Touch.h>
#include <PwmBacklight.h>

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    auto configuration = std::make_unique<Xpt2046Touch::Configuration>(
        TWODOTFOUR_LCD_SPI_HOST,  // Shared SPI
        GPIO_NUM_14,              // Touch CS
        GPIO_NUM_27,              // Touch IRQ (GPIO_NUM_NC if not used)
        240,
        320
    );

    return std::make_shared<Xpt2046Touch>(std::move(configuration));
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

    configuration->mirrorX = true;  // Adjust as needed
    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    return std::make_shared<Ili934xDisplay>(std::move(configuration));
}
