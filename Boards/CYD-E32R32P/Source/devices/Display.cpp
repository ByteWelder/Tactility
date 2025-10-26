#include "Display.h"

#include <Xpt2046Touch.h>
#include <St7789Display.h>
#include <PwmBacklight.h>
#include <Tactility/hal/touch/TouchDevice.h>

// Create the XPT2046 touch device (hardware/esp_lcd driver)
static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    auto config = std::make_unique<Xpt2046Touch::Configuration>(
        CYD_DISPLAY_SPI_HOST,                                // spi device / bus (SPI2_HOST)
        CYD_TOUCH_CS_PIN,                                    // touch CS (IO33)
        (uint16_t)CYD_DISPLAY_HORIZONTAL_RESOLUTION,         // x max
        (uint16_t)CYD_DISPLAY_VERTICAL_RESOLUTION,           // y max
        false,                                               // swapXy
        true,                                                // mirrorX
        true                                                 // mirrorY
    );

    return std::make_shared<Xpt2046Touch>(std::move(config));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    // Create the ST7789 display configuration
    auto configuration = St7789Display::Configuration {
        .horizontalResolution = CYD_DISPLAY_HORIZONTAL_RESOLUTION,
        .verticalResolution = CYD_DISPLAY_VERTICAL_RESOLUTION,
        .gapX = 0,
        .gapY = 0,
        .swapXY = false,
        .mirrorX = false,
        .mirrorY = false,
        .invertColor = false,
        .bufferSize = 0, 
        .touch = createTouch(),
        .backlightDutyFunction = driver::pwmbacklight::setBacklightDuty,
        .resetPin = GPIO_NUM_NC  // No reset pin control
    };

    // Create the SPI configuration
    auto spiConfiguration = std::make_shared<SpiConfiguration>(SpiConfiguration {
        .spiHost = CYD_DISPLAY_SPI_HOST,
        .csPin = CYD_DISPLAY_PIN_CS,
        .dcPin = CYD_DISPLAY_PIN_DC,
        .colorOrder = LCD_RGB_ELEMENT_ORDER_BGR
    });

    return std::make_shared<St7789Display>(configuration, spiConfiguration);
}