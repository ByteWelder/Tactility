#include "Display.h"
#include "Cst816Touch.h"

#include <Ili934xDisplay.h>
#include <PwmBacklight.h>

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    // Note: GPIO 25 for reset and GPIO 21 for interrupt?
    auto configuration = std::make_unique<Cst816sTouch::Configuration>(
        I2C_NUM_0,
        240,
        320
    );

    return std::make_shared<Cst816sTouch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    Ili934xDisplay::Configuration panel_configuration = {
        .horizontalResolution = LCD_HORIZONTAL_RESOLUTION,
        .verticalResolution = LCD_VERTICAL_RESOLUTION,
        .gapX = 0,
        .gapY = 0,
        .swapXY = false,
        .mirrorX = true,
        .mirrorY = false,
        .invertColor = false,
        .swapBytes = true,
        .bufferSize = LCD_BUFFER_SIZE,
        .touch = createTouch(),
        .backlightDutyFunction = driver::pwmbacklight::setBacklightDuty,
        .resetPin = GPIO_NUM_NC,
        .rgbElementOrder = LCD_RGB_ELEMENT_ORDER_BGR
    };

    auto spi_configuration = std::make_shared<Ili934xDisplay::SpiConfiguration>(Ili934xDisplay::SpiConfiguration {
        .spiHostDevice = LCD_SPI_HOST,
        .csPin = LCD_PIN_CS,
        .dcPin = LCD_PIN_DC,
        .pixelClockFrequency = 40'000'000,
        .transactionQueueDepth = 10
    });

    return std::make_shared<Ili934xDisplay>(panel_configuration, spi_configuration, true);
}
