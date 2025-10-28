#include "I8080St7789Display.h"
#include <Tactility/Log.h>
#include <driver/gpio.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lvgl_port.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lvgl.h>

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

I8080St7789Display::I8080St7789Display(const Configuration& config) 
    : configuration(config), lock(std::make_shared<tt::Lock>()) {
}

bool I8080St7789Display::initializeHardware() {
    TT_LOG_I(TAG, "Initializing I8080 ST7789 Display hardware...");

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

    // Create panel IO with proper callback
    esp_lcd_panel_io_i80_config_t io_cfg = {
        .cs_gpio_num = configuration.csPin,
        .pclk_hz = configuration.pixelClockFrequency,
        .trans_queue_depth = configuration.transactionQueueDepth,
        .on_color_trans_done = notify_lvgl_flush_ready,  // Use proper callback
        .user_ctx = nullptr,  // Will be set when LVGL display is created
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
    sendInitCommands();

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

    TT_LOG_I(TAG, "Display hardware initialized");
    return true;
}

void I8080St7789Display::sendInitCommands() {
    TT_LOG_I(TAG, "Sending ST7789 init commands");
    for (const auto& cmd : st7789_init_cmds) {
        esp_lcd_panel_io_tx_param(ioHandle, cmd.cmd, cmd.data, cmd.len & 0x7F);
        if (cmd.len & 0x80) {
            vTaskDelay(pdMS_TO_TICKS(120));
        }
    }
}

bool I8080St7789Display::initializeLvgl() {
    TT_LOG_I(TAG, "Initializing LVGL for ST7789 display");

    // Create LVGL display using lvgl_port
    lvgl_port_display_cfg_t display_cfg = {
        .io_handle = ioHandle,
        .panel_handle = panelHandle,
        .control_handle = nullptr,
        .buffer_size = configuration.bufferSize,
        .double_buffer = false,
        .trans_size = 0,
        .hres = 170,
        .vres = 320,
        .monochrome = false,
        .rotation = {
            .swap_xy = configuration.swapXY,
            .mirror_x = configuration.mirrorX,
            .mirror_y = configuration.mirrorY,
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .sw_rotate = false,
            .swap_bytes = true,
            .full_refresh = false,
            .direct_mode = false
        }
    };

    // Create the LVGL display
    lvglDisplay = lvgl_port_add_disp(&display_cfg);
    if (!lvglDisplay) {
        TT_LOG_E(TAG, "Failed to create LVGL display");
        return false;
    }

    // Set the gap for the ST7789 (35 pixels on X axis)
    lv_display_set_gap(lvglDisplay, configuration.gapX, configuration.gapY);
    
    // Set inversion
    if (configuration.invertColor) {
        esp_lcd_panel_invert_color(panelHandle, true);
    }

    // Update the user context in the IO handle to point to our LVGL display
    esp_lcd_panel_io_handle_event(ioHandle, lvglDisplay, nullptr);

    g_display_instance = this;
    TT_LOG_I(TAG, "LVGL display created successfully");
    return true;
}

bool I8080St7789Display::start() {
    return initializeHardware();
}

bool I8080St7789Display::stop() {
    // Turn off backlight
    gpio_set_level(configuration.backlightPin, 0);
    
    // Turn off display power
    gpio_set_level((gpio_num_t)15, 0);
    
    return true;
}

bool I8080St7789Display::startLvgl() {
    if (!initializeHardware()) {
        return false;
    }
    
    return initializeLvgl();
}

bool I8080St7789Display::stopLvgl() {
    if (lvglDisplay) {
        lvgl_port_remove_disp(lvglDisplay);
        lvglDisplay = nullptr;
    }
    return true;
}

lv_display_t* I8080St7789Display::getLvglDisplay() const {
    return lvglDisplay;
}
