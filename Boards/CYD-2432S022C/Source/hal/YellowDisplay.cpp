#include "YellowDisplay.h"
#include "YellowTouch.h"
#include "CYD2432S022CConstants.h"
#include "esp_log.h"
#include "Tactility/app/display/DisplaySettings.h"
#include "PwmBacklight.h"
#include <esp_lvgl_port.h>
#include <esp_heap_caps.h>

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
    ESP_LOGW(TAG, "[LOG] Entered YellowDisplay::start() at %s:%d", __FILE__, __LINE__);
    esp_backtrace_print(10); // Print up to 10 stack frames for debugging

    if (isStarted) {
        ESP_LOGW(TAG, "Display already started");
        return true;
    }

    // Log heap before LVGL init
    ESP_LOGI(TAG, "DMA heap free: %lu", static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_DMA)));
    ESP_LOGI(TAG, "Heap free before LVGL init: %lu", static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_DEFAULT)));


    ESP_LOGI(TAG, "Heap free after LVGL init: %lu", static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_DEFAULT)));
    ESP_LOGI(TAG, "DMA heap free after LVGL init: %lu", static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_DMA)));
    
    // Copy gpio_num_t[8] to int[16] for new I80Display config (fill unused with GPIO_NUM_NC)
    int dataPins[16];
    for (int i = 0; i < 8; i++) {
        dataPins[i] = static_cast<int>(config->dataPins[i]);
    }
    for (int i = 8; i < 16; i++) {
        dataPins[i] = GPIO_NUM_NC;
    }

    // Construct I80Display::Configuration using new API
    auto i80_config = std::make_unique<I80Display::Configuration>(
        config->dcPin, // dcPin
        config->wrPin, // wrPin
        dataPins,      // dataPins (int[16])
        static_cast<unsigned int>(config->horizontalResolution),
        static_cast<unsigned int>(config->verticalResolution),
        I80Display::PanelType::ST7789, // panelType, hardcoded for this board
        8, // busWidth
        config->csPin // csPin
    );
    i80_config->resetPin = config->rstPin;
    i80_config->backlightPin = config->backlightPin;
    i80_config->pixelClockFrequency = config->pclkHz;
    i80_config->drawBufferHeight = 0; // Use default unless overridden
    i80_config->invertColor = false; // Set if needed
    i80_config->rotationMode = I80Display::RotationMode::ROTATE_0;
    i80_config->touch = config->touch;
    // Add more fields as needed for your use case

    ESP_LOGI(TAG, "Configured I80Display for %ux%u", config->horizontalResolution, config->verticalResolution);
    i80Display = std::make_unique<I80Display>(std::move(i80_config));
    if (!i80Display->start()) {
        ESP_LOGE(TAG, "Failed to initialize i80 display");
        if (lvglInitialized) {
            lvgl_port_deinit();
            lvglInitialized = false;
        }
        i80Display.reset();
        return false;
    }

    if (!driver::pwmbacklight::init(config->backlightPin)) {
        ESP_LOGE(TAG, "Failed to initialize PWM backlight");
        if (lvglInitialized) {
            lvgl_port_deinit();
            lvglInitialized = false;
        }
        i80Display.reset();
        return false;
    }

    isStarted = true;
    ESP_LOGI(TAG, "Display started successfully");

    // Backlight duty
    uint8_t backlightDuty;
    if (tt::app::display::getBacklightDuty(backlightDuty)) {
        setBacklightDuty(backlightDuty);
        ESP_LOGI(TAG, "Backlight duty set to %u", backlightDuty);
    } else {
        ESP_LOGW(TAG, "Failed to get backlight duty, using default value");
        const uint8_t defaultDuty = 50;
        setBacklightDuty(defaultDuty);
        ESP_LOGI(TAG, "Backlight duty set to default %u", defaultDuty);
    }

    return true;
}

bool YellowDisplay::stop() {
    ESP_LOGW(TAG, "[LOG] Entered YellowDisplay::stop() at %s:%d", __FILE__, __LINE__);
    esp_backtrace_print(10); // Print up to 10 stack frames for debugging

    if (!isStarted) {
        ESP_LOGW(TAG, "Display not started");
        return true;
    }

    i80Display.reset();
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
    // Use setBacklightDuty only if supported by the new driver
    if (i80Display) {
        i80Display->setBacklightDuty(backlightDuty);
        ESP_LOGI(TAG, "Backlight duty set to %u", backlightDuty);
    }
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
