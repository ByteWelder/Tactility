#include "YellowDisplay.h"
#include "YellowDisplayConstants.h"

#include <Ili934xDisplay.h>
#include <Xpt2046Touch.h>
#include <PwmBacklight.h>
#include <esp_log.h>

static const char* TAG = "display";

std::shared_ptr<Xpt2046Touch> createTouch() {
    ESP_LOGI(TAG, "Creating touch on SPI1_HOST");
    auto configuration = std::make_unique<Xpt2046Touch::Configuration>(
        SPI1_HOST,
        YELLOW_TOUCH_PIN_CS,
        YELLOW_LCD_HORIZONTAL_RESOLUTION,
        YELLOW_LCD_VERTICAL_RESOLUTION,
        false,
        true,
        false
    );
    auto touch = std::make_shared<Xpt2046Touch>(std::move(configuration));
    ESP_LOGI(TAG, "Touch created");
    return touch;
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    ESP_LOGI(TAG, "Creating touch for display");
    auto touch = createTouch();
    ESP_LOGI(TAG, "Creating ILI9341 display on SPI2_HOST");
    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        YELLOW_LCD_SPI_HOST,
        YELLOW_LCD_PIN_CS,
        YELLOW_LCD_PIN_DC,
        YELLOW_LCD_HORIZONTAL_RESOLUTION,
        YELLOW_LCD_VERTICAL_RESOLUTION,
        touch
    );
    configuration->mirrorX = true;
    configuration->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;
    auto display = std::make_shared<Ili934xDisplay>(std::move(configuration));
    ESP_LOGI(TAG, "Display created");
    return display;
}
