#include "Ssd1306Display.h"

#include <Tactility/Log.h>
#include <esp_lcd_panel_commands.h>
#include <esp_lcd_panel_dev.h>
#include <esp_lcd_panel_ssd1306.h>
#include <esp_lvgl_port.h>
#include <esp_lcd_panel_ops.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

constexpr auto TAG = "SSD1306";

bool Ssd1306Display::createIoHandle(esp_lcd_panel_io_handle_t& outHandle) {
    TT_LOG_I(TAG, "Creating I2C IO handle");

    // Give the display time to power up
    vTaskDelay(pdMS_TO_TICKS(200));

    const esp_lcd_panel_io_i2c_config_t panel_io_config = {
        .dev_addr = configuration->deviceAddress,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .flags = {
            .dc_low_on_data = false,
            .disable_control_phase = false,
        },
    };

    esp_err_t ret = esp_lcd_new_panel_io_i2c(
        (esp_lcd_i2c_bus_handle_t)configuration->port, 
        &panel_io_config, 
        &outHandle
    );
    
    if (ret != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create I2C panel IO. Error code: 0x%X (%s)", ret, esp_err_to_name(ret));
        return false;
    }

    TT_LOG_I(TAG, "I2C panel IO created successfully");
    return true;
}

bool Ssd1306Display::createPanelHandle(esp_lcd_panel_io_handle_t ioHandle, esp_lcd_panel_handle_t& panelHandle) {
    TT_LOG_I(TAG, "Creating SSD1306 panel handle");

    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = static_cast<uint8_t>(configuration->verticalResolution)
    };

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = configuration->resetPin,
        .color_space = ESP_LCD_COLOR_SPACE_MONOCHROME,
        .bits_per_pixel = 1,
        .flags = {
            .reset_active_high = false
        },
        .vendor_config = &ssd1306_config
    };

    esp_err_t ret = esp_lcd_new_panel_ssd1306(ioHandle, &panel_config, &panelHandle);
    
    if (ret != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create SSD1306 panel. Error code: 0x%X (%s)", ret, esp_err_to_name(ret));
        return false;
    }

    TT_LOG_I(TAG, "SSD1306 panel created");

    // Reset the panel
    ret = esp_lcd_panel_reset(panelHandle);
    if (ret != ESP_OK) {
        TT_LOG_E(TAG, "Panel reset failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Initialize the panel using ESP-IDF driver
    ret = esp_lcd_panel_init(panelHandle);
    if (ret != ESP_OK) {
        TT_LOG_E(TAG, "Panel init failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Apply Heltec-specific hardware configuration
    // The Heltec v3 needs segment remap enabled (mirror X)
    TT_LOG_I(TAG, "Applying Heltec-specific display configuration");
    ret = esp_lcd_panel_mirror(panelHandle, true, false);
    if (ret != ESP_OK) {
        TT_LOG_E(TAG, "Mirror configuration failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Invert colors to get white on black
    ret = esp_lcd_panel_invert_color(panelHandle, !configuration->invertColor);
    if (ret != ESP_OK) {
        TT_LOG_E(TAG, "Color inversion failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Apply gap offsets if needed
    if (configuration->gapX != 0 || configuration->gapY != 0) {
        ret = esp_lcd_panel_set_gap(panelHandle, configuration->gapX, configuration->gapY);
        if (ret != ESP_OK) {
            TT_LOG_E(TAG, "Set gap failed: %s", esp_err_to_name(ret));
            return false;
        }
    }
    
    // Turn on the display
    ret = esp_lcd_panel_disp_on_off(panelHandle, true);
    if (ret != ESP_OK) {
        TT_LOG_E(TAG, "Display on failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    TT_LOG_I(TAG, "Panel initialization complete");
    return true;
}

lvgl_port_display_cfg_t Ssd1306Display::getLvglPortDisplayConfig(esp_lcd_panel_io_handle_t ioHandle, esp_lcd_panel_handle_t panelHandle) {
    TT_LOG_I(TAG, "LVGL config: %ux%u buffer=%u", 
        configuration->horizontalResolution, 
        configuration->verticalResolution,
        configuration->bufferSize);

    lvgl_port_display_cfg_t config = {
        .io_handle = ioHandle,
        .panel_handle = panelHandle,
        .control_handle = nullptr,
        .buffer_size = configuration->bufferSize,
        .double_buffer = false,
        .trans_size = 0,
        .hres = configuration->horizontalResolution,
        .vres = configuration->verticalResolution,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .color_format = LV_COLOR_FORMAT_I1,
        .flags = {
            .buff_dma = false,
            .buff_spiram = false,
            .sw_rotate = false,
            .swap_bytes = false,
            .full_refresh = false,
            .direct_mode = false
        }
    };

    TT_LOG_I(TAG, "LVGL config ready");
    return config;
}
