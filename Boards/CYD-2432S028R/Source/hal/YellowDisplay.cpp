#include "YellowDisplay.h"
#include "YellowDisplayConstants.h"
#include <Ili934xDisplay.h>
#include <Xpt2046Touch.h>
#include <PwmBacklight.h>

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    auto configuration = std::make_unique<Xpt2046Touch::Configuration>(
        CYD_TOUCH_SPI_HOST,
        CYD_TOUCH_PIN_CS,
        CYD_TOUCH_PIN_IRQ,
        CYD_DISPLAY_HORIZONTAL_RESOLUTION,
        CYD_DISPLAY_VERTICAL_RESOLUTION
    );
    configuration->spi_config = {  // Add custom SPI settings
        .mosi_io_num = GPIO_NUM_32,
        .miso_io_num = GPIO_NUM_39,
        .sclk_io_num = GPIO_NUM_25,
        .max_transfer_sz = 4096,
        .spi_mode = SPI_MODE0,
        .pclk_hz = 2000000  // JSON’s 2MHz
    };
    return std::make_shared<Xpt2046Touch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();
    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        CYD_DISPLAY_SPI_HOST,
        CYD_DISPLAY_PIN_CS,
        CYD_DISPLAY_PIN_DC,
        CYD_DISPLAY_HORIZONTAL_RESOLUTION,
        CYD_DISPLAY_VERTICAL_RESOLUTION,
        touch
    );
    configuration->mirrorX = true;
    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;
    return std::make_shared<Ili934xDisplay>(std::move(configuration));
}
