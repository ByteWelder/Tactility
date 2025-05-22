#include "YellowDisplay.h"
#include "YellowTouch.h"
#include "CYD2432S022CConstants.h"
#include <i80Display.h>
#include <PwmBacklight.h>

#include "esp_log.h"
#include <inttypes.h>

using namespace tt::hal::display;

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    // If you have a custom touch config, put it here. Otherwise, just return nullptr or use createYellowTouch().
    return createYellowTouch();
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    ESP_LOGI("YellowDisplay", "[LOG] Entered createDisplay() at %s:%d", __FILE__, __LINE__);
    auto touch = createTouch();
    int dataPins[8] = {
        CYD_2432S022C_LCD_PIN_D0,
        CYD_2432S022C_LCD_PIN_D1,
        CYD_2432S022C_LCD_PIN_D2,
        CYD_2432S022C_LCD_PIN_D3,
        CYD_2432S022C_LCD_PIN_D4,
        CYD_2432S022C_LCD_PIN_D5,
        CYD_2432S022C_LCD_PIN_D6,
        CYD_2432S022C_LCD_PIN_D7
    };
    ESP_LOGI("YellowDisplay", "Display config:");
    ESP_LOGI("YellowDisplay", "  dataPins: [%d %d %d %d %d %d %d %d]", 
        dataPins[0], dataPins[1], dataPins[2], dataPins[3],
        dataPins[4], dataPins[5], dataPins[6], dataPins[7]);
    ESP_LOGI("YellowDisplay", "  dcPin: %d, wrPin: %d, csPin: %d, rstPin: %d, backlightPin: %d", 
        CYD_2432S022C_LCD_PIN_DC, CYD_2432S022C_LCD_PIN_WR, CYD_2432S022C_LCD_PIN_CS, CYD_2432S022C_LCD_PIN_RST, CYD_2432S022C_LCD_PIN_BACKLIGHT);
    ESP_LOGI("YellowDisplay", "  Resolution: %ux%u, Pixel Clock: %u Hz, Bus Width: %u", 
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION, CYD_2432S022C_LCD_VERTICAL_RESOLUTION, CYD_2432S022C_LCD_PCLK_HZ, 8);

    auto config = std::make_unique<Configuration>();
    config->dcPin = CYD_2432S022C_LCD_PIN_DC;
    config->wrPin = CYD_2432S022C_LCD_PIN_WR;
    config->csPin = CYD_2432S022C_LCD_PIN_CS;
    config->resetPin = CYD_2432S022C_LCD_PIN_RST;
    config->horizontalResolution = CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION;
    config->verticalResolution = CYD_2432S022C_LCD_VERTICAL_RESOLUTION;
    config->panelType = PanelType::ST7789;
    config->busWidth = 8;
    config->pixelClockFrequency = CYD_2432S022C_LCD_PCLK_HZ;
    config->backlightPin = GPIO_NUM_NC;
    config->backlightDutyFunction = driver::pwmbacklight::setBacklightDuty;
    config->invertColor = false;
    config->rotationMode = RotationMode::ROTATE_0;
    config->touch = touch;
    config->supportsGammaCorrection = true;
    config->rgbElementOrder = LCD_RGB_ELEMENT_ORDER_RGB;
    config->bitsPerPixel = 16;
    config->useDmaBuffer = true;
    config->useSpiRamBuffer = false;
    config->drawBufferHeight = 20; // Use smaller buffer height to reduce memory requirements
    
    return std::make_shared<I80Display>(std::move(config));
}
