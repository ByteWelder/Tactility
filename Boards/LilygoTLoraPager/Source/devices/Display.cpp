#include "Display.h"

#include <PwmBacklight.h>
#include <St7796Display.h>

#define TPAGER_LCD_SPI_HOST SPI2_HOST
#define TPAGER_LCD_PIN_CS GPIO_NUM_38
#define TPAGER_LCD_PIN_DC GPIO_NUM_37 // RS
#define TPAGER_LCD_HORIZONTAL_RESOLUTION 222
#define TPAGER_LCD_VERTICAL_RESOLUTION 480
#define TPAGER_LCD_SPI_TRANSFER_HEIGHT (TPAGER_LCD_VERTICAL_RESOLUTION / 10)

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

    auto display = std::make_shared<St7796Display>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
