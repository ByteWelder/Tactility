#include "Display.h"

#include <PwmBacklight.h>
#include <St7789Display.h>

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    // Configuration based on demo code:
    // - Resolution: 172x320
    // - X offset: 34 pixels (gapX parameter)
    // - Y offset: 0 pixels
    // - Mirror X-axis disabled (to fix inverted text)
    // - 12MHz SPI clock

    St7789Display::Configuration panel_configuration = {
        .horizontalResolution = LCD_HORIZONTAL_RESOLUTION,
        .verticalResolution = LCD_VERTICAL_RESOLUTION,
        .gapX = LCD_GAP_X,
        .gapY = LCD_GAP_Y,
        .swapXY = false,
        .mirrorX = false,  // disabled to fix inverted text
        .mirrorY = false,
        .invertColor = true,
        .bufferSize = 0,  // default = 1/10 of screen
        .touch = nullptr,
        .backlightDutyFunction = driver::pwmbacklight::setBacklightDuty,
        .resetPin = LCD_PIN_RESET
    };

    auto spi_configuration = std::make_shared<St7789Display::SpiConfiguration>(St7789Display::SpiConfiguration {
        .spiHostDevice = LCD_SPI_HOST,
        .csPin = LCD_PIN_CS,
        .dcPin = LCD_PIN_DC,
        .pixelClockFrequency = LCD_PIXEL_CLOCK_HZ,
        .transactionQueueDepth = 10
    });

    return std::make_shared<St7789Display>(panel_configuration, spi_configuration);
}
