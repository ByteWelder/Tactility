#include "Display.h"

#include <PwmBacklight.h>
#include <St7789Display.h>

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {

    auto configuration = std::make_unique<St7789Display::Configuration>(
        SPI2_HOST,
        GPIO_NUM_39,
        GPIO_NUM_38,
        240,
        240,
        nullptr,
        false,
        false,
        false,
        true
    );


    configuration->resetPin = GPIO_NUM_42;
    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    auto display = std::make_shared<St7789Display>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
