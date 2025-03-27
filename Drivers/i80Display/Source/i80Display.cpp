#include "i80Display.h"
#include "Tactility/Log.h"
#include <esp_lcd_panel_commands.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lvgl_port.h>
#include <esp_lcd_panel_st7789.h>
#include <esp_lcd_ili9341.h>
#include <esp_heap_caps.h>

#define TAG "i80display"

namespace tt::hal::display {

bool I80Display::start() {
    TT_LOG_I(TAG, "Starting");

    // Step 1: Initialize I80 bus
    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = configuration->dcPin,
        .wr_gpio_num = configuration->wrPin,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_gpio_nums = {
            configuration->dataPins[0], configuration->dataPins[1], configuration->dataPins[2], configuration->dataPins[3],
            configuration->dataPins[4], configuration->dataPins[5], configuration->dataPins[6], configuration->dataPins[7],
            configuration->dataPins[8], configuration->dataPins[9], configuration->dataPins[10], configuration->dataPins[11],
            configuration->dataPins[12], configuration->dataPins[13], configuration->dataPins[14], configuration->dataPins[15]
        },
        .bus_width = configuration->busWidth,
        .max_transfer_bytes = configuration->horizontalResolution * configuration->verticalResolution * 2, // Full screen in RGB565
        .dma_burst_size = 64,
        .psram_trans_align = 0,
        .sram_trans_align = 0,
    };
    if (esp_lcd_new_i80_bus(&bus_config, &i80Bus) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create I80 bus");
        return false;
    }

    // Step 2: Initialize panel I/O
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = configuration->csPin,
        .pclk_hz = configuration->pixelClockFrequency,
        .trans_queue_depth = configuration->transactionQueueDepth,
    `   .on_color_trans_done = nullptr,
        .user_ctx = nullptr,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_levels = { 
            .dc_idle_level = 0, 
            .dc_cmd_level = 0, 
            .dc_dummy_level = 0, 
            .dc_data_level = 1 
        },
        .flags = { 
            .cs_active_high = 0, 
            .reverse_color_bits = 0, 
            .swap_color_bytes = 0,
            .pclk_active_neg = 0, 
            .pclk_idle_low = 0 
        },
    };
    if (esp_lcd_new_panel_io_i80(i80Bus, &io_config, &ioHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create panel IO");
        esp_lcd_del_i80_bus(i80Bus);
        return false;
    }

    // Step 3: Initialize panel based on type
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = configuration->resetPin,
        .rgb_ele_order = configuration->rgbElementOrder,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = 16,
        .flags = { .reset_active_high = 0 },
        .vendor_config = nullptr
    };
    esp_err_t ret;
    switch (configuration->panelType) {
        case PanelType::ST7789:
            ret = esp_lcd_new_panel_st7789(ioHandle, &panel_config, &panelHandle);
            break;
        case PanelType::ILI9341:
            ret = esp_lcd_new_panel_ili9341(ioHandle, &panel_config, &panelHandle);
            break;
        default:
            TT_LOG_E(TAG, "Unsupported panel type");
            return false;
    }
    if (ret != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create panel");
        esp_lcd_del_i80_bus(i80Bus);
        return false;
    }

    // Step 4: Configure panel
    if (configuration->resetPin != GPIO_NUM_NC) {
        if (esp_lcd_panel_reset(panelHandle) != ESP_OK) {
            TT_LOG_E(TAG, "Failed to reset panel");
            return false;
        }
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
        TT_LOG_E(TAG, "Failed to set mirror");
        return false;
    }
    if (esp_lcd_panel_invert_color(panelHandle, configuration->invertColor) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set invert color");
        return false;
    }
    if (esp_lcd_panel_disp_on_off(panelHandle, true) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to turn display on");
        return false;
    }

    // Step 5: Set up LVGL display
    uint32_t buffer_size = configuration->bufferSize;
    if (buffer_size == 0) {
        buffer_size = configuration->horizontalResolution * configuration->verticalResolution / 10;
    }
    lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = ioHandle,
        .panel_handle = panelHandle,
        .control_handle = nullptr,
        .buffer_size = buffer_size,
        .double_buffer = false,
        .trans_size = 0,
        .hres = configuration->horizontalResolution,
        .vres = configuration->verticalResolution,
        .monochrome = false,
        .rotation = { configuration->swapXY, configuration->mirrorX, configuration->mirrorY },
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

bool I80Display::stop() {
    if (!displayHandle) {
        TT_LOG_W(TAG, "Display not started");
        return true;
    }

    lvgl_port_remove_disp(displayHandle);
    displayHandle = nullptr;

    if (esp_lcd_panel_del(panelHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to delete panel");
        return false;
    }
    panelHandle = nullptr;

    if (esp_lcd_panel_io_del(ioHandle) != ESP_OK) {  // Fixed function name
        TT_LOG_E(TAG, "Failed to delete panel IO");
        return false;
    }
    ioHandle = nullptr;

    if (esp_lcd_del_i80_bus(i80Bus) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to delete I80 bus");
        return false;
    }
    i80Bus = nullptr;

    TT_LOG_I(TAG, "Stopped");
    return true;
}

void I80Display::setGammaCurve(uint8_t index) {
    uint8_t gamma_curve;
    switch (index) {
        case 0: gamma_curve = 0x01; break;
        case 1: gamma_curve = 0x04; break;
        case 2: gamma_curve = 0x02; break;
        case 3: gamma_curve = 0x08; break;
        default: return;
    }
    const uint8_t param[] = { gamma_curve };
    if (esp_lcd_panel_io_tx_param(ioHandle, LCD_CMD_GAMSET, param, 1) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to set gamma");
    }
}

} // namespace tt::hal::display
