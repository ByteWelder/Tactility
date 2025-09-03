#include "Display.h"

#include <Ili934xDisplay.h>
#include <Xpt2046Touch.h>

#include <PwmBacklight.h>

std::shared_ptr<Xpt2046Touch> createTouch() {
    auto configuration = std::make_unique<Xpt2046Touch::Configuration>(
        CROWPANEL_LCD_SPI_HOST,
        CROWPANEL_TOUCH_PIN_CS,
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
        CROWPANEL_LCD_SPI_HOST,
        CROWPANEL_LCD_PIN_CS,
        CROWPANEL_LCD_PIN_DC,
        240,
        320,
        touch
    );

    configuration->mirrorX = true;
    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    auto display = std::make_shared<Ili934xDisplay>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
