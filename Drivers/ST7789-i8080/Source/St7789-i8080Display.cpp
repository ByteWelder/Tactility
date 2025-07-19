#include "esp_log.h"
#include "St7789-i8080Display.h"
#include "esp_lcd_panel_st7796.h"

static const char* TAG = "St7789I8080";

// Minimal ST7789 initialization sequence
const st7796_lcd_init_cmd_t St7789I8080Display::st7789_init_cmds_[] = {
    // Sleep out - enable normal operation  
    {0x11, {0}, 0, 120},                    // SLPOUT + 120ms delay
    
    // Configure 16 bpp (65k colors) display mode
    {0x3A, {0x05}, 1, 0},                   // COLMOD - 16 bits per pixel
    
    // Memory Data Access Control (can be modified for orientation)
    {0x36, {0x00}, 1, 0},                   // MADCTL - normal orientation (will be updated)
    
    // Enable color inversion for IPS displays (if needed)
    // This will be conditionally applied based on config
    
    // Display on
    {0x29, {0}, 0, 0},                      // DISPON
};

St7789I8080Display::St7789I8080Display(std::unique_ptr<Configuration> config) 
    : config_(std::move(config)), panel_handle_(nullptr), io_handle_(nullptr), 
      i80_bus_(nullptr), initialized_(false) {
    
    ESP_LOGI(TAG, "Initializing ST7789 I8080 Display");
    ESP_LOGI(TAG, "Resolution: %dx%d, Bus Width: %d-bit", 
             config_->horizontal_resolution, config_->vertical_resolution, config_->bus_width);
    
    initializeI8080Bus();
    initializePanelIO(); 
    initializePanel();
    applyDisplaySettings();
    initializeBacklight();
    
    initialized_ = true;
    ESP_LOGI(TAG, "ST7789 Display initialization complete");
}

St7789I8080Display::~St7789I8080Display() {
    if (panel_handle_) {
        esp_lcd_panel_del(panel_handle_);
    }
    if (io_handle_) {
        esp_lcd_panel_io_del(io_handle_);
    }
    if (i80_bus_) {
        esp_lcd_del_i80_bus(i80_bus_);
    }
    
    // Turn off backlight
    if (config_->pin_backlight != GPIO_NUM_NC) {
        gpio_set_level(config_->pin_backlight, !config_->backlight_on_level);
    }
}

void St7789I8080Display::initializeI8080Bus() {
    ESP_LOGI(TAG, "Initialize Intel 8080 bus");
    
    esp_lcd_i80_bus_config_t bus_config = {};
    bus_config.clk_src = LCD_CLK_SRC_DEFAULT;
    bus_config.dc_gpio_num = config_->pin_dc;
    bus_config.wr_gpio_num = config_->pin_wr;
    
    // Set data pins
    for (int i = 0; i < config_->bus_width; i++) {
        bus_config.data_gpio_nums[i] = config_->data_pins[i];
    }
    
    bus_config.bus_width = config_->bus_width;
    bus_config.max_transfer_bytes = config_->horizontal_resolution * config_->vertical_resolution * 2; // 16-bit pixels
    bus_config.psram_trans_align = 64;
    bus_config.sram_trans_align = 4;
    
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus_));
}

void St7789I8080Display::initializePanelIO() {
    ESP_LOGI(TAG, "Install panel IO");
    
    esp_lcd_panel_io_i80_config_t io_config = {};
    io_config.cs_gpio_num = config_->pin_cs;
    io_config.pclk_hz = config_->pixel_clock_hz;
    io_config.trans_queue_depth = 10;
    io_config.on_color_trans_done = displayNotifyCallback;
    io_config.user_ctx = this;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    io_config.dc_levels.dc_idle_level = 0;
    io_config.dc_levels.dc_cmd_level = 0;
    io_config.dc_levels.dc_dummy_level = 0;
    io_config.dc_levels.dc_data_level = 1;
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus_, &io_config, &io_handle_));
}

void St7789I8080Display::initializePanel() {
    ESP_LOGI(TAG, "Install ST7789 panel driver");
    
    // Prepare custom initialization commands
    std::vector<st7796_lcd_init_cmd_t> init_cmds(
        st7789_init_cmds_, 
        st7789_init_cmds_ + sizeof(st7789_init_cmds_) / sizeof(st7796_lcd_init_cmd_t)
    );
    
    // Add color inversion if needed
    if (config_->invert_colors) {
        st7796_lcd_init_cmd_t invert_cmd = {0x21, {0}, 0, 0}; // INVON
        init_cmds.insert(init_cmds.end() - 1, invert_cmd); // Insert before DISPON
    }
    
    st7796_vendor_config_t vendor_config = {};
    vendor_config.init_cmds = init_cmds.data();
    vendor_config.init_cmds_size = init_cmds.size();
    
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = config_->pin_rst;
    panel_config.rgb_endian = LCD_RGB_ENDIAN_RGB;
    panel_config.bits_per_pixel = 16;
    panel_config.vendor_config = &vendor_config;
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io_handle_, &panel_config, &panel_handle_));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle_));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle_));
}

void St7789I8080Display::applyDisplaySettings() {
    // Apply mirror and swap settings
    if (config_->mirror_x || config_->mirror_y || config_->swap_xy) {
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle_, config_->mirror_x, config_->mirror_y));
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle_, config_->swap_xy));
    }
    
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle_, true));
}

void St7789I8080Display::initializeBacklight() {
    if (config_->pin_backlight == GPIO_NUM_NC) {
        ESP_LOGI(TAG, "No backlight pin configured");
        return;
    }
    
    ESP_LOGI(TAG, "Initializing backlight on GPIO %d", config_->pin_backlight);
    
    gpio_config_t backlight_config = {};
    backlight_config.pin_bit_mask = 1ULL << config_->pin_backlight;
    backlight_config.mode = GPIO_MODE_OUTPUT;
    backlight_config.pull_up_en = GPIO_PULLUP_DISABLE;
    backlight_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    backlight_config.intr_type = GPIO_INTR_DISABLE;
    
    ESP_ERROR_CHECK(gpio_config(&backlight_config));
    setBacklight(true);
}

void St7789I8080Display::setBacklight(bool on) {
    if (config_->pin_backlight != GPIO_NUM_NC) {
        bool level = on ? config_->backlight_on_level : !config_->backlight_on_level;
        ESP_ERROR_CHECK(gpio_set_level(config_->pin_backlight, level));
    }
}

bool St7789I8080Display::displayNotifyCallback(esp_lcd_panel_io_handle_t panel_io, 
                                              esp_lcd_panel_io_event_data_t *edata, 
                                              void *user_ctx) {
    return false;
}

// Drawing functions
void St7789I8080Display::setPixel(uint16_t x, uint16_t y, uint16_t color) {
    esp_lcd_panel_draw_bitmap(panel_handle_, x, y, x + 1, y + 1, &color);
}

void St7789I8080Display::fillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    size_t pixel_count = width * height;
    uint16_t* buffer = (uint16_t*)malloc(pixel_count * sizeof(uint16_t));
    if (buffer) {
        for (size_t i = 0; i < pixel_count; i++) {
            buffer[i] = color;
        }
        esp_lcd_panel_draw_bitmap(panel_handle_, x, y, x + width, y + height, buffer);
        free(buffer);
    }
}

void St7789I8080Display::drawBitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* bitmap) {
    esp_lcd_panel_draw_bitmap(panel_handle_, x, y, x + width, y + height, bitmap);
}

void St7789I8080Display::clearScreen(uint16_t color) {
    fillRect(0, 0, getWidth(), getHeight(), color);
}
