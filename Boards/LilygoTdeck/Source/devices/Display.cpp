#include "Display.h"

#include <Gt911Touch.h>
#include <PwmBacklight.h>
#include <St7789Display.h>

#define TDECK_LCD_SPI_HOST SPI2_HOST
#define TDECK_LCD_PIN_CS GPIO_NUM_12
#define TDECK_LCD_PIN_DC GPIO_NUM_11 // RS
#define TDECK_LCD_HORIZONTAL_RESOLUTION 320
#define TDECK_LCD_VERTICAL_RESOLUTION 240
#define TDECK_LCD_SPI_TRANSFER_HEIGHT (TDECK_LCD_VERTICAL_RESOLUTION / 10)

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    // Note for future changes: Reset pin is 48 and interrupt pin is 47
    auto configuration = std::make_unique<Gt911Touch::Configuration>(
        I2C_NUM_0,
        240,
        320,
        true,
        true,
        false
    );

    return std::make_shared<Gt911Touch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    St7789Display::Configuration panel_configuration = {
        .swapXY = true,
        .mirrorX = true,
        .mirrorY = false,
        .invertColor = true,
        .touch = createTouch(),
        .backlightDutyFunction = driver::pwmbacklight::setBacklightDuty
    };

    auto spi_configuration = std::make_shared<St7789Display::SpiConfiguration>(St7789Display::SpiConfiguration {
        .spiHostDevice = TDECK_LCD_SPI_HOST,
        .csPin = TDECK_LCD_PIN_CS,
        .dcPin = TDECK_LCD_PIN_DC,
        .pixelClockFrequency = 80'000'000,
        .transactionQueueDepth = 10
    });

    auto display = std::make_shared<St7789Display>(panel_configuration, spi_configuration);
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
