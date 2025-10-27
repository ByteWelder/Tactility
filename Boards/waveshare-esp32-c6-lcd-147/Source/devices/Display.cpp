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
        .horizontalResolution = 172,
        .verticalResolution = 320,
        .gapX = 34,  // X offset for 1.47" display
        .gapY = 0,
        .swapXY = false,
        .mirrorX = false,  // disabled to fix inverted text
        .mirrorY = false,
        .invertColor = true,
        .bufferSize = 0,  // default = 1/10 of screen
        .touch = nullptr,
        .backlightDutyFunction = driver::pwmbacklight::setBacklightDuty,
        .resetPin = GPIO_NUM_21
    };

    auto spi_configuration = std::make_shared<St7789Display::SpiConfiguration>(St7789Display::SpiConfiguration {
        .spiHostDevice = SPI2_HOST,
        .csPin = GPIO_NUM_14,
        .dcPin = GPIO_NUM_15,
        .pixelClockFrequency = 12'000'000,  // 12 MHz as in demo
        .transactionQueueDepth = 10
    });

    return std::make_shared<St7789Display>(panel_configuration, spi_configuration);
}
