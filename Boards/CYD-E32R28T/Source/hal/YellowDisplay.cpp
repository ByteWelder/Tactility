#include "YellowDisplay.h"
#include "YellowDisplayConstants.h"
#include "Xpt2046Touch.h"
#include <Ili934xDisplay.h>
#include <PwmBacklight.h>
#include <Tactility/hal/touch/TouchDevice.h>
#include <esp_log.h>
#include <memory>

static const char* TAG = "YellowDisplay";

// Global to hold reference if needed (is this needed?)
static std::shared_ptr<Xpt2046Touch> touch;

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    ESP_LOGI(TAG, "Creating SPI touch");

    // Create configuration object
    auto config = std::make_unique<Xpt2046Touch::Configuration>(
        CYD_DISPLAY_SPI_HOST,   // SPI bus handle
        CYD_TOUCH_CS_PIN,       // Chip select
        CYD_DISPLAY_HORIZONTAL_RESOLUTION, // xMax
        CYD_DISPLAY_VERTICAL_RESOLUTION,   // yMax
        false,  // swapXY
        true,   // mirrorX
        false   // mirrorY
    );

    // Allocate driver
    touch = std::make_shared<Xpt2046Touch>(std::move(config));

    // No explicit start() required for Xpt2046Touch
    return std::static_pointer_cast<tt::hal::touch::TouchDevice>(touch);
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch_device = createTouch();
    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        CYD_DISPLAY_SPI_HOST,
        CYD_DISPLAY_PIN_CS,
        CYD_DISPLAY_PIN_DC,
        CYD_DISPLAY_HORIZONTAL_RESOLUTION,
        CYD_DISPLAY_VERTICAL_RESOLUTION,
        touch_device
    );
    configuration->mirrorX = true;
    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;
    configuration->rgbElementOrder = LCD_RGB_ELEMENT_ORDER_BGR;

    return std::make_shared<Ili934xDisplay>(std::move(configuration));
}
