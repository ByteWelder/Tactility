#include "ST7789Display.h"
#include "CST820Touch.h"
#include "CYD2432S022CConstants.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "ST7789Display";

ST7789Display::ST7789Display(std::unique_ptr<Configuration> config)
    : config_(std::move(config)) {}

bool ST7789Display::initialize_hardware() {
    ESP_LOGI(TAG, "Initializing ST7789 display hardware...");

    // Configure GPIO pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CYD_2432S022C_LCD_PIN_D0) | (1ULL << CYD_2432S022C_LCD_PIN_D1) |
                        (1ULL << CYD_2432S022C_LCD_PIN_D2) | (1ULL << CYD_2432S022C_LCD_PIN_D3) |
                        (1ULL << CYD_2432S022C_LCD_PIN_D4) | (1ULL << CYD_2432S022C_LCD_PIN_D5) |
                        (1ULL << CYD_2432S022C_LCD_PIN_D6) | (1ULL << CYD_2432S022C_LCD_PIN_D7) |
                        (1ULL << CYD_2432S022C_LCD_PIN_WR) | (1ULL << CYD_2432S022C_LCD_PIN_RD) |
                        (1ULL << CYD_2432S022C_LCD_PIN_RS) | (1ULL << CYD_2432S022C_LCD_PIN_CS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO pins");
        return false;
    }

    // ST7789 initialization sequence
    ESP_LOGI(TAG, "Starting ST7789 display initialization");
    gpio_set_level(CYD_2432S022C_LCD_PIN_CS, 0);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // Software reset
    ESP_LOGI(TAG, "Sending software reset (SWRESET)");
    write_byte(0x01);
    vTaskDelay(pdMS_TO_TICKS(150));

    // Sleep out
    ESP_LOGI(TAG, "Sending sleep out (SLPOUT)");
    write_byte(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));

    // Color mode: 16-bit RGB565
    ESP_LOGI(TAG, "Setting color mode to RGB565 (COLMOD)");
    write_byte(0x3A);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    write_byte(0x55); // RGB565
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // Memory access control (orientation)
    ESP_LOGI(TAG, "Setting memory access control (MADCTL)");
    write_byte(0x36);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    write_byte(0x00); // Normal orientation (adjust if needed: 0x60 for 90°, 0xC0 for 180°, 0xA0 for 270°)
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // Display inversion on (optional, depends on your display)
    ESP_LOGI(TAG, "Enabling display inversion (INVON)");
    write_byte(0x21);

    // Normal display mode on
    ESP_LOGI(TAG, "Setting normal display mode (NORON)");
    write_byte(0x13);

    // Display on
    ESP_LOGI(TAG, "Turning display on (DISPON)");
    write_byte(0x29);
    ESP_LOGI(TAG, "Display on, delaying 10ms");
    vTaskDelay(pdMS_TO_TICKS(10));

    gpio_set_level(CYD_2432S022C_LCD_PIN_CS, 1);
    ESP_LOGI(TAG, "ST7789 initialization complete");

    return true;
}

void ST7789Display::write_byte(uint8_t data) {
    gpio_set_level(CYD_2432S022C_LCD_PIN_WR, 0);
    const gpio_num_t pins[8] = {
        CYD_2432S022C_LCD_PIN_D0,
        CYD_2432S022C_LCD_PIN_D1,
        CYD_2432S022C_LCD_PIN_D2,
        CYD_2432S022C_LCD_PIN_D3,
        CYD_2432S022C_LCD_PIN_D4,
        CYD_2432S022C_LCD_PIN_D5,
        CYD_2432S022C_LCD_PIN_D6,
        CYD_2432S022C_LCD_PIN_D7
    };
    for (int i = 0; i < 8; i++) {
        gpio_set_level(pins[i], (data >> i) & 1);
    }
    gpio_set_level(CYD_2432S022C_LCD_PIN_WR, 1);
}

void ST7789Display::set_address_window(int x, int y, int w, int h) {
    gpio_set_level(CYD_2432S022C_LCD_PIN_CS, 0);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // Set column address
    write_byte(0x2A);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    write_byte((x >> 8) & 0xFF);
    write_byte(x & 0xFF);
    write_byte(((x + w - 1) >> 8) & 0xFF);
    write_byte((x + w - 1) & 0xFF);

    // Set row address
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0);
    write_byte(0x2B);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1);
    write_byte((y >> 8) & 0xFF);
    write_byte(y & 0xFF);
    write_byte(((y + h - 1) >> 8) & 0xFF);
    write_byte((y + h - 1) & 0xFF);

    // Memory write
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0);
    write_byte(0x2C);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1);
}

bool ST7789Display::start() {
    ESP_LOGI(TAG, "Starting ST7789 display...");
    if (!initialize_hardware()) {
        ESP_LOGE(TAG, "Failed to initialize ST7789 display hardware");
        return false;
    }
    hardware_initialized_ = true;
    return true;
}

bool ST7789Display::stop() {
    ESP_LOGI(TAG, "Stopping ST7789 display...");
    if (display_) {
        lv_display_delete(display_);
        display_ = nullptr;
    }
    hardware_initialized_ = false;
    return true;
}

lv_display_t* ST7789Display::getLvglDisplay() const {
    if (!hardware_initialized_) {
        ESP_LOGE(TAG, "Cannot get LVGL display: hardware not initialized");
        return nullptr;
    }

    if (!display_) {
        // Remove the default display created by initEspLvglPort
        lv_display_t* default_disp = lv_disp_get_default();
        if (default_disp) {
            lv_display_delete(default_disp);
            ESP_LOGI(TAG, "Removed default display created by initEspLvglPort");
        }

        // Create our own display
        display_ = lv_display_create(config_->width, config_->height);
        if (!display_) {
            ESP_LOGE(TAG, "Failed to create LVGL display");
            return nullptr;
        }

        // Set up the draw buffer (required for LVGL to render)
        static lv_color_t buf[CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION * 10]; // Buffer for 10 rows
        lv_display_set_buffers(display_, buf, nullptr, CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION * 10, LV_DISPLAY_RENDER_MODE_PARTIAL);

        // Set the flush callback
        lv_display_set_flush_cb(display_, [](lv_display_t* disp, const lv_area_t* area, unsigned char* color_p) {
            auto* display_device = static_cast<ST7789Display*>(lv_display_get_user_data(disp));
            if (display_device) {
                display_device->flush(area, color_p);
            } else {
                ESP_LOGE(TAG, "Failed to get ST7789Display from user data");
                lv_display_flush_ready(disp);
            }
        });
    }
    return display_;
}

void ST7789Display::flush(const lv_area_t* area, unsigned char* color_p) {
    if (!hardware_initialized_ || !display_) {
        ESP_LOGE(TAG, "Cannot flush: display not initialized");
        return;
    }

    ESP_LOGI(TAG, "Flushing area x1=%d, y1=%d, x2=%d, y2=%d", area->x1, area->y1, area->x2, area->y2);

    int w = (area->x2 - area->x1 + 1);
    int h = (area->y2 - area->y1 + 1);

    set_address_window(area->x1, area->y1, w, h);

    // Convert ARGB8888 (32-bit) to RGB565 (16-bit) manually
    ESP_LOGI(TAG, "Writing %d pixels (w=%d, h=%d)", w * h, w, h);
    for (int i = 0; i < w * h; i++) {
        // Assuming color_p is in ARGB8888 format (4 bytes per pixel: A, R, G, B)
        uint8_t r = color_p[i * 4 + 1]; // Red
        uint8_t g = color_p[i * 4 + 2]; // Green
        uint8_t b = color_p[i * 4 + 3]; // Blue

        // Convert to RGB565
        uint16_t r5 = (r >> 3) & 0x1F; // 5 bits for red
        uint16_t g6 = (g >> 2) & 0x3F; // 6 bits for green
        uint16_t b5 = (b >> 3) & 0x1F; // 5 bits for blue
        uint16_t rgb565 = (r5 << 11) | (g6 << 5) | b5;

        // Split into high and low bytes
        uint8_t high = (rgb565 >> 8) & 0xFF;
        uint8_t low = rgb565 & 0xFF;

        // Log the first few pixels for debugging
        if (i < 4) {
            ESP_LOGI(TAG, "Pixel %d: R=%02x, G=%02x, B=%02x -> RGB565 high=%02x, low=%02x", i, r, g, b, high, low);
        }

        // Write to the display
        write_byte(high);
        write_byte(low);
    }

    gpio_set_level(CYD_2432S022C_LCD_PIN_CS, 1);
    lv_display_flush_ready(display_);
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createCST820Touch();

    auto configuration = std::make_unique<ST7789Display::Configuration>(
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION,
        touch
    );

    return std::make_shared<ST7789Display>(std::move(configuration));
}
