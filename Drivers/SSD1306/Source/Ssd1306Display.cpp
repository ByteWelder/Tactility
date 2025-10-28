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

// SSD1306 Commands
#define SSD1306_CMD_DISPLAY_OFF           0xAE
#define SSD1306_CMD_DISPLAY_ON            0xAF
#define SSD1306_CMD_SET_CLOCK_DIV         0xD5
#define SSD1306_CMD_SET_MUX_RATIO         0xA8
#define SSD1306_CMD_SET_DISPLAY_OFFSET    0xD3
#define SSD1306_CMD_SET_START_LINE        0x40
#define SSD1306_CMD_CHARGE_PUMP           0x8D
#define SSD1306_CMD_MEM_ADDR_MODE         0x20
#define SSD1306_CMD_SEG_REMAP             0xA1
#define SSD1306_CMD_COM_SCAN_DEC          0xC8
#define SSD1306_CMD_COM_PINS              0xDA
#define SSD1306_CMD_SET_CONTRAST          0x81
#define SSD1306_CMD_SET_PRECHARGE         0xD9
#define SSD1306_CMD_SET_VCOMH             0xDB
#define SSD1306_CMD_NORMAL_DISPLAY        0xA6
#define SSD1306_CMD_DISPLAYALLON_RESUME   0xA4
#define SSD1306_CMD_DEACTIVATE_SCROLL     0x2E

// I2C control bytes
#define I2C_CONTROL_BYTE_CMD_SINGLE       0x80

static esp_err_t ssd1306_i2c_send_cmd(i2c_port_t port, uint8_t addr, uint8_t cmd) {
    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    if (!handle) return ESP_ERR_NO_MEM;
    
    i2c_master_start(handle);
    i2c_master_write_byte(handle, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(handle, I2C_CONTROL_BYTE_CMD_SINGLE, true);
    i2c_master_write_byte(handle, cmd, true);
    i2c_master_stop(handle);
    esp_err_t ret = i2c_master_cmd_begin(port, handle, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(handle);
    return ret;
}

static esp_err_t ssd1306_send_init_sequence(i2c_port_t port, uint8_t addr, uint8_t height) {
    TT_LOG_I(TAG, "Sending SSD1306 init sequence for %d height...", height);
    
    ssd1306_i2c_send_cmd(port, addr, SSD1306_CMD_DISPLAY_OFF);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    ssd1306_i2c_send_cmd(port, addr, SSD1306_CMD_SET_CLOCK_DIV);
    ssd1306_i2c_send_cmd(port, addr, 0x80);
    
    ssd1306_i2c_send_cmd(port, addr, SSD1306_CMD_SET_MUX_RATIO);
    ssd1306_i2c_send_cmd(port, addr, height - 1);
    
    ssd1306_i2c_send_cmd(port, addr, SSD1306_CMD_SET_DISPLAY_OFFSET);
    ssd1306_i2c_send_cmd(port, addr, 0x00);
    
    ssd1306_i2c_send_cmd(port, addr, SSD1306_CMD_SET_START_LINE);
    
    ssd1306_i2c_send_cmd(port, addr, SSD1306_CMD_CHARGE_PUMP);
    ssd1306_i2c_send_cmd(port, addr, 0x14);
    
    ssd1306_i2c_send_cmd(port, addr, SSD1306_CMD_MEM_ADDR_MODE);
    ssd1306_i2c_send_cmd(port, addr, 0x00);  // Horizontal mode
    
    ssd1306_i2c_send_cmd(port, addr, SSD1306_CMD_SEG_REMAP | 0x01);
    ssd1306_i2c_send_cmd(port, addr, SSD1306_CMD_COM_SCAN_DEC);
    
    ssd1306_i2c_send_cmd(port, addr, SSD1306_CMD_COM_PINS);
    ssd1306_i2c_send_cmd(port, addr, height == 64 ? 0x12 : 0x02);
    
    ssd1306_i2c_send_cmd(port, addr, SSD1306_CMD_SET_CONTRAST);
    ssd1306_i2c_send_cmd(port, addr, 0xCF);
    
    ssd1306_i2c_send_cmd(port, addr, SSD1306_CMD_SET_PRECHARGE);
    ssd1306_i2c_send_cmd(port, addr, 0xF1);
    
    ssd1306_i2c_send_cmd(port, addr, SSD1306_CMD_SET_VCOMH);
    ssd1306_i2c_send_cmd(port, addr, 0x40);
    
    ssd1306_i2c_send_cmd(port, addr, SSD1306_CMD_NORMAL_DISPLAY);
    ssd1306_i2c_send_cmd(port, addr, SSD1306_CMD_DISPLAY_ON);

    TT_LOG_I(TAG, "Init sequence complete");
    return ESP_OK;
}

bool Ssd1306Display::createIoHandle(esp_lcd_panel_io_handle_t& outHandle) {
    TT_LOG_I(TAG, "Creating I2C IO handle");

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

    // Hardware reset manually
    if (configuration->resetPin != GPIO_NUM_NC) {
        gpio_config_t rst_cfg = {
            .pin_bit_mask = 1ULL << configuration->resetPin,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&rst_cfg);
        
        gpio_set_level(configuration->resetPin, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(configuration->resetPin, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Send our custom init sequence via I2C
    ssd1306_send_init_sequence(configuration->port, configuration->deviceAddress, 
                               configuration->verticalResolution);
    
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