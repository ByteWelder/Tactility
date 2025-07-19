#include "YellowDisplay.h"
#include "YellowTouch.h"
#include "CYD2432S022CConstants.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include <esp_lcd_panel_st7796.h>
#include "driver/gpio.h"
#include <memory>

static const char* TAG = "YellowDisplay";

// Minimal ST7789 initialization sequence (recommended by lcamtuf)
static const st7796_lcd_init_cmd_t st7789_minimal_init_cmds[] = {
    // Step 1: Hardware reset is handled by esp_lcd_panel_reset()
    
    // Step 2: Sleep out - enable normal operation
    {0x11, {0}, 0, 120},                    // SLPOUT + 120ms delay (conservative timing)
    
    // Step 3: Configure 16 bpp (65k colors) display mode  
    {0x3A, {0x05}, 1, 0},                   // COLMOD - 16 bits per pixel (0x05 = 16bpp, 0x06 = 18bpp)
    
    // Step 4: Enable color inversion (necessary for IPS displays)
    {0x21, {0}, 0, 0},                      // INVON - Invert colors for IPS TFT
    
    // Step 5: Turn display on
    {0x29, {0}, 0, 0},                      // DISPON - Display on
    
    // Note: Step 6 (RAMWR 0x2C) is handled by the drawing functions
};

// Callback function for panel IO
static bool display_notify_callback(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    // Handle any panel IO events if needed
    return false;
}

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    return createYellowTouch();
}

class ST7796DisplayDevice {
private:
    esp_lcd_panel_handle_t panel_handle;
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_i80_bus_handle_t i80_bus;
    std::shared_ptr<tt::hal::touch::TouchDevice> touch_device;
    gpio_num_t backlight_pin;

public:
    ST7796DisplayDevice(std::shared_ptr<tt::hal::touch::TouchDevice> touch) 
        : panel_handle(nullptr), io_handle(nullptr), i80_bus(nullptr), 
          touch_device(touch), backlight_pin(CYD_2432S022C_LCD_PIN_BACKLIGHT) {
        
        initializeDisplay();
        initializeBacklight();
    }

    ~ST7796DisplayDevice() {
        if (panel_handle) {
            esp_lcd_panel_del(panel_handle);
        }
        if (io_handle) {
            esp_lcd_panel_io_del(io_handle);
        }
        if (i80_bus) {
            esp_lcd_del_i80_bus(i80_bus);
        }
        
        // Turn off backlight
        if (backlight_pin != GPIO_NUM_NC) {
            gpio_set_level(backlight_pin, !CYD_2432S022C_LCD_BACKLIGHT_ON_LEVEL);
        }
    }

private:
    void initializeDisplay() {
        ESP_LOGI(TAG, "Initializing ST7789 display with minimal command sequence");
        
        // Configure I80 bus
        esp_lcd_i80_bus_config_t bus_config = {};
        bus_config.clk_src = LCD_CLK_SRC_DEFAULT;
        bus_config.dc_gpio_num = CYD_2432S022C_LCD_PIN_DC;
        bus_config.wr_gpio_num = CYD_2432S022C_LCD_PIN_WR;
        bus_config.data_gpio_nums[0] = CYD_2432S022C_LCD_PIN_D0;
        bus_config.data_gpio_nums[1] = CYD_2432S022C_LCD_PIN_D1;
        bus_config.data_gpio_nums[2] = CYD_2432S022C_LCD_PIN_D2;
        bus_config.data_gpio_nums[3] = CYD_2432S022C_LCD_PIN_D3;
        bus_config.data_gpio_nums[4] = CYD_2432S022C_LCD_PIN_D4;
        bus_config.data_gpio_nums[5] = CYD_2432S022C_LCD_PIN_D5;
        bus_config.data_gpio_nums[6] = CYD_2432S022C_LCD_PIN_D6;
        bus_config.data_gpio_nums[7] = CYD_2432S022C_LCD_PIN_D7;
        bus_config.bus_width = CYD_2432S022C_LCD_BUS_WIDTH;
        bus_config.max_transfer_bytes = CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * 2; // 16-bit pixels
        bus_config.psram_trans_align = 64;
        bus_config.sram_trans_align = 4;

        ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

        // Configure panel IO
        esp_lcd_panel_io_i80_config_t io_config = {};
        io_config.cs_gpio_num = CYD_2432S022C_LCD_PIN_CS;
        io_config.pclk_hz = CYD_2432S022C_LCD_PCLK_HZ;
        io_config.trans_queue_depth = 10;
        io_config.on_color_trans_done = display_notify_callback;
        io_config.user_ctx = this;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        io_config.dc_levels.dc_idle_level = 0;
        io_config.dc_levels.dc_cmd_level = 0;
        io_config.dc_levels.dc_dummy_level = 0;
        io_config.dc_levels.dc_data_level = 1;

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));

        // Configure ST7796 panel with minimal ST7789 init commands
        st7796_vendor_config_t vendor_config = {};
        vendor_config.init_cmds = st7789_minimal_init_cmds;
        vendor_config.init_cmds_size = sizeof(st7789_minimal_init_cmds) / sizeof(st7796_lcd_init_cmd_t);

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = CYD_2432S022C_LCD_PIN_RST;
        panel_config.rgb_endian = LCD_RGB_ENDIAN_RGB;
        panel_config.bits_per_pixel = 16;
        panel_config.vendor_config = &vendor_config;

        ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle));

        // Step 1: Hardware reset (gets all registers in predictable state)
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
        
        // Steps 2-5: Initialize with minimal command sequence
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
        
        // Panel is now ready - the init commands already turned display on
        ESP_LOGI(TAG, "ST7789 display initialized with minimal command sequence");
        ESP_LOGI(TAG, "Display ready for drawing (16bpp, color inverted for IPS)");
    }

    void initializeBacklight() {
        if (backlight_pin == GPIO_NUM_NC) {
            ESP_LOGI(TAG, "No backlight pin configured");
            return;
        }

        ESP_LOGI(TAG, "Initializing backlight on GPIO %d", backlight_pin);
        
        gpio_config_t backlight_config = {};
        backlight_config.pin_bit_mask = 1ULL << backlight_pin;
        backlight_config.mode = GPIO_MODE_OUTPUT;
        backlight_config.pull_up_en = GPIO_PULLUP_DISABLE;
        backlight_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
        backlight_config.intr_type = GPIO_INTR_DISABLE;
        
        ESP_ERROR_CHECK(gpio_config(&backlight_config));
        ESP_ERROR_CHECK(gpio_set_level(backlight_pin, CYD_2432S022C_LCD_BACKLIGHT_ON_LEVEL));
        
        ESP_LOGI(TAG, "Backlight enabled");
    }

public:
    // Public interface methods
    void setPixel(uint16_t x, uint16_t y, uint16_t color) {
        // Step 6: RAMWR (0x2C) is handled internally by esp_lcd_panel_draw_bitmap
        esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + 1, y + 1, &color);
    }

    void fillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
        // Create buffer filled with color
        size_t pixel_count = width * height;
        uint16_t* buffer = (uint16_t*)malloc(pixel_count * sizeof(uint16_t));
        if (buffer) {
            for (size_t i = 0; i < pixel_count; i++) {
                buffer[i] = color;
            }
            esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + width, y + height, buffer);
            free(buffer);
        }
    }

    void drawBitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* bitmap) {
        // RAMWR (0x2C) handled internally
        esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + width, y + height, bitmap);
    }

    void clearScreen(uint16_t color = 0x0000) {
        fillRect(0, 0, getWidth(), getHeight(), color);
    }

    std::shared_ptr<tt::hal::touch::TouchDevice> getTouch() {
        return touch_device;
    }

    uint16_t getWidth() { return CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION; }
    uint16_t getHeight() { return CYD_2432S022C_LCD_VERTICAL_RESOLUTION; }
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    ESP_LOGI(TAG, "[LOG] Entered createDisplay() at %s:%d", __FILE__, __LINE__);
    
    auto touch = createTouch();
    
    ESP_LOGI(TAG, "Display config:");
    ESP_LOGI(TAG, "  Resolution: %ux%u", 
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION, CYD_2432S022C_LCD_VERTICAL_RESOLUTION);
    ESP_LOGI(TAG, "  Pixel Clock: %u Hz, Bus Width: %u", 
        CYD_2432S022C_LCD_PCLK_HZ, CYD_2432S022C_LCD_BUS_WIDTH);
    ESP_LOGI(TAG, "  Control pins - DC:%d, WR:%d, CS:%d, RST:%d, BL:%d", 
        CYD_2432S022C_LCD_PIN_DC, CYD_2432S022C_LCD_PIN_WR, 
        CYD_2432S022C_LCD_PIN_CS, CYD_2432S022C_LCD_PIN_RST, 
        CYD_2432S022C_LCD_PIN_BACKLIGHT);
    ESP_LOGI(TAG, "  Using minimal ST7789 initialization sequence");

    // Create and return the display device
    return std::make_shared<ST7796DisplayDevice>(touch);
}
