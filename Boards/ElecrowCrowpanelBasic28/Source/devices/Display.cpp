#include "Display.h"

#include <Ili934xDisplay.h>
#include <Xpt2046Touch.h>

#include <PwmBacklight.h>

std::shared_ptr<Xpt2046Touch> createTouch() {
    auto configuration = std::make_unique<Xpt2046Touch::Configuration>(
        LCD_SPI_HOST,
        TOUCH_PIN_CS,
        LCD_HORIZONTAL_RESOLUTION,
        LCD_VERTICAL_RESOLUTION,
        false,
        true,
        false
    );

    return std::make_shared<Xpt2046Touch>(std::move(configuration));
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
