#include "TdeckDisplay.h"
#include "TdeckDisplayConstants.h"
#include "TdeckTouch.h"

#include <PwmBacklight.h>
#include <St7789Display.h>

#include <driver/spi_master.h>

#define TAG "tdeck_display"

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = std::make_shared<TdeckTouch>();

    auto configuration = std::make_unique<St7789Display::Configuration>(
        TDECK_LCD_SPI_HOST,
        TDECK_LCD_PIN_CS,
        TDECK_LCD_PIN_DC,
        320,
        240,
        touch
    );

    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    return std::make_shared<St7789Display>(std::move(configuration));
}
