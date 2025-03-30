#include "YellowDisplay.h"
#include "YellowDisplayConstants.h"
#include <Ili934xDisplay.h>
#include <Xpt2046Touch.h>
#include <PwmBacklight.h>

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    auto configuration = std::make_unique<Xpt2046Touch::Configuration>(
        CYD_TOUCH_SPI_HOST,
        CYD_TOUCH_PIN_CS,
        CYD_TOUCH_PIN_IRQ,
        CYD_DISPLAY_HORIZONTAL_RESOLUTION,
        CYD_DISPLAY_VERTICAL_RESOLUTION
    );
    return std::make_shared<Xpt2046Touch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();
    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        CYD_DISPLAY_SPI_HOST,
        CYD_DISPLAY_PIN_CS,
        CYD_DISPLAY_PIN_DC,
        CYD_DISPLAY_HORIZONTAL_RESOLUTION,
        CYD_DISPLAY_VERTICAL_RESOLUTION,
        touch
    );

    configuration->mirrorX = true; // Per JSON: DISPLAY_MIRROR_X=true
    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    return std::make_shared<Ili934xDisplay>(std::move(configuration));
}
