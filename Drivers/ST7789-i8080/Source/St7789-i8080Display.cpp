#include "St7789-i8080Display.h"
#include <Tactility/Log.h>
#include <esp_lcd_panel_commands.h>
#include <esp_lcd_panel_dev.h>
#include <esp_lcd_st7796.h>
#include <esp_lvgl_port.h>

#define TAG "st7789-i8080"

// Custom initialization sequence
static const st7796_lcd_init_cmd_t st7789_init_cmds_[] = {
    {0x11, nullptr, 0, 120},                 // SLPOUT + 120ms delay
    {0x3A, (uint8_t[]){0x05}, 1, 0},        // COLMOD - 16 bits per pixel
    {0x36, (uint8_t[]){0x00}, 1, 0},        // MADCTL
    {0x29, nullptr, 0, 0},                   // DISPON
};

bool St7789I8080Display::start() {
    TT_LOG_I(TAG, "Starting");

    // Initialize I8080 bus
    TT_LOG_I(TAG, "Initialize Intel 8080 bus");
    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = configuration->pin_dc,
        .wr_gpio_num = configuration->pin_wr,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_gpio_nums = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .bus_width = configuration->busWidth,
        .max_transfer_bytes = static_cast<size_t>(configuration->horizontalResolution * configuration->verticalResolution * 2),
    };
    for (int i = 0; i < configuration->busWidth; i++) {
        bus_config.data_gpio_nums[i] = configuration->dataPins[i];
    }

    if (esp_lcd_new_i80_bus(&bus_config, &i80Bus) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create I8080 bus");
        return false;
    }

    // Initialize panel IO
    TT_LOG_I(TAG, "Install panel IO");
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = configuration->pin_cs,
        .pclk_hz = configuration->pixelClockHz,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1
        },
    };

    if (esp_lcd_new_panel_io_i80(i80Bus, &io_config, &ioHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create panel IO");
        return false;
    }

    // Initialize panel
    TT_LOG_I(TAG, "Install ST7796 panel driver");
    std::vector<st7796_lcd_init_cmd_t> init_cmds(st7789_init_cmds_, st7789_init_cmds_ + sizeof(st7789_init_cmds_) / sizeof(st7796_lcd_init_cmd_t));
    if (configuration->invertColor) {
        st7796_lcd_init_cmd_t invert_cmd = {0x21, nullptr, 0, 0}; // INVON
        init_cmds.insert(init_cmds.end() - 1, invert_cmd);
    }

    st7796_vendor_config_t vendor_config = {
        .init_cmds = init_cmds.data(),
        .init_cmds_size = static_cast<uint16_t>(init_cmds.size())
    };
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = configuration->pin_rst,
        .rgb_endian = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_config
    };

    if (esp_lcd_new_panel_st7796(ioHandle, &panel_config, &panelHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create panel");
        return false;
    }

    if (esp_lcd_panel_reset(panelHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to reset panel");
        return false;
    }

    if (esp_lcd_panel_init(panelHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to init panel");
        return false;
    }

    if (esp_lcd_panel_swap_xy(panelHandle, configuration->swapXY) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to swap XY");
        return false;
    }

    if (esp_lcd_panel_mirror(panelHandle, configuration->mirrorX, configuration->mirrorY) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set panel to mirror");
        return false;
    }

    if (esp_lcd_panel_disp_on_off(panelHandle, true) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to turn display on");
        return false;
    }

    // Initialize backlight
    if (configuration->pin_backlight != GPIO_NUM_NC) {
        TT_LOG_I(TAG, "Initializing backlight on GPIO %d", configuration->pin_backlight);
        gpio_config_t backlight_config = {
            .pin_bit_mask = 1ULL << configuration->pin_backlight,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        if (gpio_config(&backlight_config) != ESP_OK) {
            TT_LOG_E(TAG, "Failed to configure backlight GPIO");
            return false;
        }
        setBacklight(true);
    }

    // Initialize LVGL display
    uint32_t buffer_size;
    if (configuration->bufferSize == 0) {
        buffer_size = configuration->horizontalResolution * configuration->verticalResolution / 10;
    } else {
        buffer_size = configuration->bufferSize;
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = ioHandle,
        .panel_handle = panelHandle,
        .control_handle = nullptr,
        .buffer_size = buffer_size,
        .double_buffer = false,
        .trans_size = 0,
        .hres = configuration->horizontalResolution,
        .vres = configuration->verticalResolution,
        .monochrome = false,
        .rotation = {
            .swap_xy = configuration->swapXY,
            .mirror_x = configuration->mirrorX,
            .mirror_y = configuration->mirrorY
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .sw_rotate = false,
            .swap_bytes = false,
            .full_refresh = false,
            .direct_mode = false
        }
    };

    displayHandle = lvgl_port_add_disp(&disp_cfg);

    TT_LOG_I(TAG, "Finished");
    return displayHandle != nullptr;
}

bool St7789I8080Display::stop() {
    assert(displayHandle != nullptr);

    lvgl_port_remove_disp(displayHandle);

    if (esp_lcd_panel_del(panelHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to delete panel");
        return false;
    }

    if (esp_lcd_panel_io_del(ioHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to delete panel IO");
        return false;
    }

    if (esp_lcd_del_i80_bus(i80Bus) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to delete I8080 bus");
        return false;
    }

    if (configuration->pin_backlight != GPIO_NUM_NC) {
        gpio_set_level(configuration->pin_backlight, !configuration->backlightOnLevel);
    }

    displayHandle = nullptr;
    return true;
}

void St7789I8080Display::setBacklight(bool on) {
    if (configuration->pin_backlight != GPIO_NUM_NC) {
        bool level = on ? configuration->backlightOnLevel : !configuration->backlightOnLevel;
        ESP_ERROR_CHECK(gpio_set_level(configuration->pin_backlight, level));
    }
}

void St7789I8080Display::setGammaCurve(uint8_t index) {
    uint8_t gamma_curve;
    switch (index) {
        case 0:
            gamma_curve = 0x01;
            break;
        case 1:
            gamma_curve = 0x04;
            break;
        case 2:
            gamma_curve = 0x02;
            break;
        case 3:
            gamma_curve = 0x08;
            break;
        default:
            return;
    }
    const uint8_t param[] = { gamma_curve };

    if (esp_lcd_panel_io_tx_param(ioHandle, LCD_CMD_GAMSET, param, 1) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set gamma");
    }
}
