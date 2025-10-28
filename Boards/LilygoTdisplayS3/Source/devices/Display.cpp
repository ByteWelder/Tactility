#include "Display.h"
#include <Tactility/Log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_lcd_panel_io.h>
#include <driver/gpio.h>
#include <lvgl.h>
#include <array>
#include <cstdint>
#include <cstring>

constexpr auto TAG = "I8080St7789Display";
static I8080St7789Display* g_display_instance = nullptr;
static DRAM_ATTR uint8_t buf1[170 * 320 / 10 * LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)];

// ST7789 initialization commands
typedef struct {
    uint8_t cmd;
    uint8_t data[14];
    uint8_t len;
} lcd_init_cmd_t;

static const lcd_init_cmd_t st7789_init_cmds[] = {
    {0x11, {0}, 0 | 0x80},
    {0x36, {0x08}, 1},  
    {0x3A, {0X05}, 1},
    {0x20, {0}, 0},  
    {0xB2, {0X0B, 0X0B, 0X00, 0X33, 0X33}, 5},
    {0xB7, {0X75}, 1},
    {0xBB, {0X28}, 1},
    {0xC0, {0X2C}, 1},
    {0xC2, {0X01}, 1},
    {0xC3, {0X1F}, 1},
    {0xC6, {0X13}, 1},
    {0xD0, {0XA7}, 1},
    {0xD0, {0XA4, 0XA1}, 2},
    {0xD6, {0XA1}, 1},
    {0xE0, {0XF0, 0X05, 0X0A, 0X06, 0X06, 0X03, 0X2B, 0X32, 0X43, 0X36, 0X11, 0X10, 0X2B, 0X32}, 14},
    {0xE1, {0XF0, 0X08, 0X0C, 0X0B, 0X09, 0X24, 0X2B, 0X22, 0X43, 0X38, 0X15, 0X16, 0X2F, 0X37}, 14},
};

// Callback when color transfer is done
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, 
                                    esp_lcd_panel_io_event_data_t *edata, 
                                    void *user_ctx) {
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

bool I8080St7789Display::initialize(lv_display_t* lvglDisplayCtx) {
    TT_LOG_I(TAG, "Initializing I8080 ST7789 Display...");

    // Power on the display first!
    gpio_config_t pwr_gpio_cfg = {
        .pin_bit_mask = 1ULL << 15,  // PIN_POWER_ON
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&pwr_gpio_cfg);
    gpio_set_level((gpio_num_t)15, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    TT_LOG_I(TAG, "Display power enabled");

    // Configure RD pin
    if (configuration.rdPin != GPIO_NUM_NC) {
        gpio_config_t rd_gpio_config = {
            .pin_bit_mask = (1ULL << static_cast<uint32_t>(configuration.rdPin)),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&rd_gpio_config);
        gpio_set_level(configuration.rdPin, 1);
    }

    // Create I80 bus
    size_t max_bytes = configuration.bufferSize * sizeof(uint16_t);
    esp_lcd_i80_bus_config_t bus_cfg = {
        .dc_gpio_num = configuration.dcPin,
        .wr_gpio_num = configuration.wrPin,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_gpio_nums = {
            configuration.dataPins[0], configuration.dataPins[1], 
            configuration.dataPins[2], configuration.dataPins[3],
            configuration.dataPins[4], configuration.dataPins[5], 
            configuration.dataPins[6], configuration.dataPins[7],
        },
        .bus_width = 8,
        .max_transfer_bytes = max_bytes,
        .psram_trans_align = 64,
        .sram_trans_align = 4
    };
    
    if (esp_lcd_new_i80_bus(&bus_cfg, &i80BusHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create I80 bus");
        return false;
    }

    // Create panel IO - don't use callback, let LVGL handle it
    esp_lcd_panel_io_i80_config_t io_cfg = {
        .cs_gpio_num = configuration.csPin,
        .pclk_hz = configuration.pixelClockFrequency,
        .trans_queue_depth = configuration.transactionQueueDepth,
        .on_color_trans_done = nullptr,  // No callback - LVGL will handle flush
        .user_ctx = nullptr,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .flags = {
            .cs_active_high = 0,
            .reverse_color_bits = 0,
            .swap_color_bytes = 1,
            .pclk_active_neg = 0,
            .pclk_idle_low = 0
        }
    };

    if (esp_lcd_new_panel_io_i80(i80BusHandle, &io_cfg, &ioHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to create panel IO");
        return false;
    }

    // Hardware reset
    if (configuration.resetPin != GPIO_NUM_NC) {
        gpio_config_t rst_gpio_cfg = {
            .pin_bit_mask = 1ULL << static_cast<uint32_t>(configuration.resetPin),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&rst_gpio_cfg);
        
        gpio_set_level(configuration.resetPin, 0);
        vTaskDelay(pdMS_TO_TICKS(20));
        gpio_set_level(configuration.resetPin, 1);
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    // Send initialization commands
    TT_LOG_I(TAG, "Sending ST7789 init commands");
    for (const auto& cmd : st7789_init_cmds) {
        esp_lcd_panel_io_tx_param(ioHandle, cmd.cmd, cmd.data, cmd.len & 0x7F);
        if (cmd.len & 0x80) {
            vTaskDelay(pdMS_TO_TICKS(120));
        }
    }

    // Configure backlight
    gpio_config_t bk_gpio_cfg = {
        .pin_bit_mask = 1ULL << static_cast<uint32_t>(configuration.backlightPin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&bk_gpio_cfg);
    gpio_set_level(configuration.backlightPin, 1);

    g_display_instance = this;

    TT_LOG_I(TAG, "Display hardware initialized");
    return true;
}

// LVGL command callback
static void st7789_send_cmd_cb(lv_display_t*, const uint8_t* cmd, size_t, const uint8_t* param, size_t param_size) {
    if (g_display_instance && g_display_instance->getIoHandle()) {
        esp_lcd_panel_io_tx_param(g_display_instance->getIoHandle(), *cmd, param, param_size);
    }
}

// LVGL color data callback
static void st7789_send_color_cb(lv_display_t* disp, const uint8_t* cmd, size_t, uint8_t* param, size_t param_size) {
    if (!g_display_instance || !g_display_instance->getIoHandle()) {
        return;
    }
    
    if (!disp) {
        TT_LOG_E(TAG, "Display context is NULL in color callback!");
        return;
    }
    
    esp_lcd_panel_io_tx_color(g_display_instance->getIoHandle(), *cmd, param, param_size);

    vTaskDelay(1);  // Small delay to ensure command is processed fully

    // Call flush_ready immediately - the DMA will handle the actual transfer
    lv_display_flush_ready(disp);
}

bool I8080St7789Display::startLvgl() {
    TT_LOG_I(TAG, "Starting LVGL for ST7789 display");

    // Initialize hardware FIRST without display context
    if (!ioHandle) {
        TT_LOG_I(TAG, "Hardware not initialized, calling initialize()");
        if (!initialize(nullptr)) {  // Pass nullptr for now
            TT_LOG_E(TAG, "Hardware initialization failed");
            return false;
        }
    }

    TT_LOG_I(TAG, "Creating LVGL ST7789 display");

    lvglDisplay = lv_st7789_create(170, 320, LV_LCD_FLAG_NONE,
        st7789_send_cmd_cb,
        st7789_send_color_cb
    );

    if (!lvglDisplay) {
        TT_LOG_E(TAG, "Failed to create LVGL ST7789 display");
        return false;
    }

    // Configure LVGL display
    lv_display_set_color_format(lvglDisplay, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(lvglDisplay, buf1, nullptr, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    // Rotation 0 works!
    lv_display_set_rotation(lvglDisplay, LV_DISPLAY_ROTATION_0);
    
    lv_st7789_set_gap(lvglDisplay, 35, 0);
    
    lv_st7789_set_invert(lvglDisplay, true);

    TT_LOG_I(TAG, "LVGL ST7789 display created successfully");
    return true;
}

lv_display_t* I8080St7789Display::getLvglDisplay() const {
    return lvglDisplay;
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto display = std::make_shared<I8080St7789Display>(
        I8080St7789Display::Configuration(
            GPIO_NUM_6,   // CS
            GPIO_NUM_7,   // DC
            GPIO_NUM_8,   // WR
            GPIO_NUM_9,   // RD
            { GPIO_NUM_39, GPIO_NUM_40, GPIO_NUM_41, GPIO_NUM_42,
              GPIO_NUM_45, GPIO_NUM_46, GPIO_NUM_47, GPIO_NUM_48 }, // D0..D7
            GPIO_NUM_5,   // RST
            GPIO_NUM_38   // BL
        )
    );

    return display;
}