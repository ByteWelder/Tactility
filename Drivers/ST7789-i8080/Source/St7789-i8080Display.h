#pragma once

#include <memory>
#include "esp_lcd_panel_handle.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_i80_interface.h"
#include "driver/gpio.h"

namespace tt::hal::display {
    class TouchDevice;
    class DisplayDevice;
}

class St7789I8080Display {
public:
    struct Configuration {
        // I8080 Bus configuration
        gpio_num_t pin_wr;
        gpio_num_t pin_dc;
        gpio_num_t pin_cs;
        gpio_num_t pin_rst;
        gpio_num_t pin_backlight;
        
        // Data pins (8-bit or 16-bit)
        gpio_num_t data_pins[16];
        uint8_t bus_width;  // 8 or 16
        
        // Display properties
        uint16_t horizontal_resolution;
        uint16_t vertical_resolution;
        uint32_t pixel_clock_hz;
        
        // Touch device
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
        
        // Display orientation/mirroring
        bool mirror_x = false;
        bool mirror_y = false;
        bool swap_xy = false;
        bool invert_colors = true;  // Usually needed for IPS displays
        
        // Backlight configuration
        bool backlight_on_level = true;  // true = active high, false = active low
        
        Configuration(
            gpio_num_t wr, gpio_num_t dc, gpio_num_t cs, gpio_num_t rst, gpio_num_t bl,
            uint16_t width, uint16_t height, uint32_t pixel_clock,
            std::shared_ptr<tt::hal::touch::TouchDevice> touch_device
        ) : pin_wr(wr), pin_dc(dc), pin_cs(cs), pin_rst(rst), pin_backlight(bl),
            horizontal_resolution(width), vertical_resolution(height), 
            pixel_clock_hz(pixel_clock), touch(touch_device), bus_width(8) {
            // Initialize data pins array
            for (int i = 0; i < 16; i++) {
                data_pins[i] = GPIO_NUM_NC;
            }
        }
        
        void setDataPins8Bit(gpio_num_t d0, gpio_num_t d1, gpio_num_t d2, gpio_num_t d3,
                            gpio_num_t d4, gpio_num_t d5, gpio_num_t d6, gpio_num_t d7) {
            bus_width = 8;
            data_pins[0] = d0; data_pins[1] = d1; data_pins[2] = d2; data_pins[3] = d3;
            data_pins[4] = d4; data_pins[5] = d5; data_pins[6] = d6; data_pins[7] = d7;
        }
    };

private:
    std::unique_ptr<Configuration> config_;
    esp_lcd_panel_handle_t panel_handle_;
    esp_lcd_panel_io_handle_t io_handle_;
    esp_lcd_i80_bus_handle_t i80_bus_;
    bool initialized_;

    // ST7789 minimal initialization commands (based on lcamtuf's recommendation)
    static const st7796_lcd_init_cmd_t st7789_init_cmds_[];

    void initializeI8080Bus();
    void initializePanelIO();
    void initializePanel();
    void initializeBacklight();
    void applyDisplaySettings();
    
    static bool displayNotifyCallback(esp_lcd_panel_io_handle_t panel_io, 
                                    esp_lcd_panel_io_event_data_t *edata, 
                                    void *user_ctx);

public:
    explicit St7789I8080Display(std::unique_ptr<Configuration> config);
    ~St7789I8080Display();

    // DisplayDevice interface
    void setPixel(uint16_t x, uint16_t y, uint16_t color);
    void fillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
    void drawBitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* bitmap);
    void clearScreen(uint16_t color = 0x0000);
    
    uint16_t getWidth() const { return config_->horizontal_resolution; }
    uint16_t getHeight() const { return config_->vertical_resolution; }
    
    std::shared_ptr<tt::hal::touch::TouchDevice> getTouch() { return config_->touch; }
    
    // Backlight control
    void setBacklight(bool on);
};

// Implementation
