#include "Display.h"
#include "Xpt2046SoftSpi.h"
#include <St7789Display.h>
#include <PwmBacklight.h>

constexpr auto* TAG = "CYD";

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    auto configuration = std::make_unique<Xpt2046SoftSpi::Configuration>(
        TOUCH_MOSI_PIN,
        TOUCH_MISO_PIN,
        TOUCH_SCK_PIN,
        TOUCH_CS_PIN,
        LCD_HORIZONTAL_RESOLUTION, // 240
        LCD_VERTICAL_RESOLUTION, // 320
        false, // swapXY
        true, // mirrorX
        false // mirrorY
    );

    // Allocate the driver
    auto touch = std::make_shared<Xpt2046SoftSpi>(std::move(configuration));
    
    // Start the driver
    if (!touch->start()) {
        ESP_LOGE(TAG, "Touch driver start failed");
        return nullptr;
    }

    return touch;
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    St7789Display::Configuration panel_configuration = {
        .horizontalResolution = LCD_HORIZONTAL_RESOLUTION,
        .verticalResolution = LCD_VERTICAL_RESOLUTION,
        .gapX = 0,
        .gapY = 0,
        .swapXY = false,
        .mirrorX = false,
        .mirrorY = false,
        .invertColor = false,
        .bufferSize = LCD_BUFFER_SIZE,
        .touch = createTouch(),
        .backlightDutyFunction = driver::pwmbacklight::setBacklightDuty,
        .resetPin = GPIO_NUM_NC
    };

    auto spi_configuration = std::make_shared<St7789Display::SpiConfiguration>(St7789Display::SpiConfiguration {
        .spiHostDevice = LCD_SPI_HOST,
        .csPin = LCD_PIN_CS,
        .dcPin = LCD_PIN_DC,
        .pixelClockFrequency = 80'000'000,
        .transactionQueueDepth = 10
    });

    return std::make_shared<St7789Display>(panel_configuration, spi_configuration);
}
