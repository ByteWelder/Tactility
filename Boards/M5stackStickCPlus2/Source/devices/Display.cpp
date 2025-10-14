#include "Display.h"

#include <PwmBacklight.h>
#include <St7789Display.h>

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto configuration = std::make_unique<St7789Display::Configuration>(
        LCD_SPI_HOST,
        LCD_PIN_CS,
        LCD_PIN_DC,
        LCD_HORIZONTAL_RESOLUTION,
        LCD_VERTICAL_RESOLUTION,
        nullptr,
        false,
        false,
        false,
        true,
        LCD_DRAW_BUFFER_SIZE,
        52,
        40
    );

    configuration->pixelClockFrequency = 40'000'000;
    configuration->resetPin = LCD_PIN_RESET;
    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    const auto display = std::make_shared<St7789Display>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
