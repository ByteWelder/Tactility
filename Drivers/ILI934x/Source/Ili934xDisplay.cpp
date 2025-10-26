#include "Ili934xDisplay.h"

#include <esp_lcd_ili9341.h>
#include <esp_lvgl_port.h>

std::shared_ptr<EspLcdConfiguration> Ili934xDisplay::createEspLcdConfiguration(const Configuration& configuration) {
    return std::make_shared<EspLcdConfiguration>(EspLcdConfiguration {
        .horizontalResolution = configuration.horizontalResolution,
        .verticalResolution = configuration.verticalResolution,
        .gapX = configuration.gapX,
        .gapY = configuration.gapY,
        .monochrome = false,
        .swapXY = configuration.swapXY,
        .mirrorX = configuration.mirrorX,
        .mirrorY = configuration.mirrorY,
        .invertColor = configuration.invertColor,
        .bufferSize = configuration.bufferSize,
        .touch = configuration.touch,
        .backlightDutyFunction = configuration.backlightDutyFunction,
        .resetPin = configuration.resetPin,
        .lvglColorFormat = LV_COLOR_FORMAT_RGB565,
        .lvglSwapBytes = configuration.swapBytes,
        .rgbElementOrder = configuration.rgbElementOrder,
        .bitsPerPixel = 16
    });
}

esp_lcd_panel_dev_config_t Ili934xDisplay::createPanelConfig(std::shared_ptr<EspLcdConfiguration> espLcdConfiguration, gpio_num_t resetPin) {
    return {
        .reset_gpio_num = resetPin,
        .rgb_ele_order = espLcdConfiguration->rgbElementOrder,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = espLcdConfiguration->bitsPerPixel,
        .flags = {
            .reset_active_high = 0
        },
        .vendor_config = nullptr
    };
}

bool Ili934xDisplay::createPanelHandle(esp_lcd_panel_io_handle_t ioHandle, const esp_lcd_panel_dev_config_t& panelConfig, esp_lcd_panel_handle_t& panelHandle) {
    return esp_lcd_new_panel_ili9341(ioHandle, &panelConfig, &panelHandle) == ESP_OK;
}
