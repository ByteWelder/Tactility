#include "Display.h"

#include <Gt911Touch.h>
#include <Ili934xDisplay.h>
#include <PwmBacklight.h>

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    auto configuration = std::make_unique<Gt911Touch::Configuration>(
        I2C_NUM_0,
        240,
        320
    );

    return std::make_shared<Gt911Touch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {

    auto touch = createTouch();

    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        SPI2_HOST,
        GPIO_NUM_15,
        GPIO_NUM_2,
        240,
        320,
        touch,
        true,
        true,
        true,
        true,
        0,
        LCD_RGB_ELEMENT_ORDER_RGB
    );

    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    auto display = std::make_shared<Ili934xDisplay>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
