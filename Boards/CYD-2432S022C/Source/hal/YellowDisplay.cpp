#include "YellowDisplay.h"
#include "YellowTouch.h"
#include "CYD2432S022CConstants.h"
#include "esp_log.h"
#include "Tactility/app/display/DisplaySettings.h"
#include "PwmBacklight.h"

#define TAG "YellowDisplay"

namespace tt::hal::display {

YellowDisplay::YellowDisplay(std::unique_ptr<Configuration> config)
    : config(std::move(config)), i80Display(nullptr), isStarted(false) {
}

YellowDisplay::~YellowDisplay() {
    if (isStarted) {
        stop();
    }
}

bool YellowDisplay::start() {
    if (isStarted) {
        ESP_LOGW(TAG, "Display already started");
        return true;
    }

    // Configure I80Display
    I80Display::Configuration i80_config(
        config->csPin,
        config->dcPin,
        config->wrPin,
        config->dataPins,
        config->horizontalResolution,
        config->verticalResolution,
        config->touch,
        I80Display::PanelType::ST7789,
        8
    );
    i80_config.resetPin = config->rstPin;
    i80_config.pixelClockFrequency = config->pclkHz;
    i80_config.bufferSize = config->bufferSize ? config->bufferSize : CYD_2432S022C_LCD_DRAW_BUFFER_SIZE;
    i80_config.swapXY = config->swapXY;
    i80_config.mirrorX = config->mirrorX;
    i80_config.mirrorY = config->mirrorY;
    i80_config.backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;

    i80Display = std::make_unique<I80Display>(std::make_unique<I80Display::Configuration>(i80_config));
    if (!i80Display->start()) {
        ESP_LOGE(TAG, "Failed to initialize I80 display");
        i80Display.reset();
        return false;
    }

    if (!driver::pwmbacklight::init(config->backlightPin)) {
        ESP_LOGE(TAG, "Failed to initialize PWM backlight");
        i80Display.reset();
        return false;
    }

    isStarted = true;
    ESP_LOGI(TAG, "Display started successfully");

    setBacklightDuty(tt::app::display::getBacklightDuty());
    return true;
}

bool YellowDisplay::stop() {
    if (!isStarted) {
        ESP_LOGW(TAG, "Display not started");
        return true;
    }

    i80Display.reset(); // Calls I80Display::stop() automatically
    isStarted = false;
    ESP_LOGI(TAG, "Display stopped successfully");
    return true;
}

std::shared_ptr<tt::hal::touch::TouchDevice> YellowDisplay::createTouch() {
    return config->touch;
}

lv_display_t* YellowDisplay::getLvglDisplay() const {
    return i80Display ? i80Display->getLvglDisplay() : nullptr;
}

void YellowDisplay::setBacklightDuty(uint8_t backlightDuty) {
    if (!isStarted) {
        ESP_LOGE(TAG, "setBacklightDuty: Display not started");
        return;
    }
    i80Display->setBacklightDuty(backlightDuty); // Uses the callback set in config
    ESP_LOGI(TAG, "Backlight duty set to %u", backlightDuty);
}

std::shared_ptr<DisplayDevice> createDisplay() {
    auto touch = createYellowTouch();
    auto config = std::make_unique<YellowDisplay::Configuration>(
        YellowDisplay::Configuration{
            .pclkHz = CYD_2432S022C_LCD_PCLK_HZ,
            .csPin = CYD_2432S022C_LCD_PIN_CS,
            .dcPin = CYD_2432S022C_LCD_PIN_DC,
            .wrPin = CYD_2432S022C_LCD_PIN_WR,
            .rstPin = CYD_2432S022C_LCD_PIN_RST,
            .backlightPin = CYD_2432S022C_LCD_PIN_BACKLIGHT,
            .dataPins = {
                CYD_2432S022C_LCD_PIN_D0, CYD_2432S022C_LCD_PIN_D1,
                CYD_2432S022C_LCD_PIN_D2, CYD_2432S022C_LCD_PIN_D3,
                CYD_2432S022C_LCD_PIN_D4, CYD_2432S022C_LCD_PIN_D5,
                CYD_2432S022C_LCD_PIN_D6, CYD_2432S022C_LCD_PIN_D7
            },
            .horizontalResolution = CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,
            .verticalResolution = CYD_2432S022C_LCD_VERTICAL_RESOLUTION,
            .bufferSize = CYD_2432S022C_LCD_DRAW_BUFFER_SIZE,
            .touch = touch
        }
    );
    config->swapXY = false;
    config->mirrorX = false;
    config->mirrorY = false;
    return std::make_shared<YellowDisplay>(std::move(config));
}

} // namespace tt::hal::display
