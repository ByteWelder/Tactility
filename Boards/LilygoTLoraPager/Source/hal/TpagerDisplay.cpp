#include "TpagerDisplay.h"
#include "TpagerDisplayConstants.h"

#include <PwmBacklight.h>
#include <St7796Display.h>

#include <driver/spi_master.h>

#define TAG "TPAGER_display"

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto configuration = std::make_unique<St7796Display::Configuration>(
        TPAGER_LCD_SPI_HOST,
        TPAGER_LCD_PIN_CS,
        TPAGER_LCD_PIN_DC,
        480, // w
        222, // h
        nullptr,
        true, //swapXY
        true, //mirrorX
        true, //mirrorY
        true, //invertColor
        0, //gapX
        49 //gapY
    );

    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    return std::make_shared<St7796Display>(std::move(configuration));
}
