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

    // Basic ST7789 initialization
    gpio_set_level(CYD_2432S022C_LCD_PIN_CS, 0);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode
    write_byte(0x11); // Sleep out
    vTaskDelay(pdMS_TO_TICKS(120));
    write_byte(0x29); // Display on
    gpio_set_level(CYD_2432S022C_LCD_PIN_CS, 1);

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
        display_ = lv_display_create(config_->width, config_->height);
        if (!display_) {
            ESP_LOGE(TAG, "Failed to create LVGL display");
            return nullptr;
        }
        lv_display_set_user_data(display_, const_cast<ST7789Display*>(this)); // Cast away const
    }
    return display_;
}

void ST7789Display::flush(const lv_area_t* area, unsigned char* color_p) {
    if (!hardware_initialized_ || !display_) {
        ESP_LOGE(TAG, "Cannot flush: display not initialized");
        return;
    }

    int w = (area->x2 - area->x1 + 1);
    int h = (area->y2 - area->y1 + 1);

    set_address_window(area->x1, area->y1, w, h);

    // Write pixel data (color_p is a buffer of RGB565 pixels, 2 bytes per pixel)
    for (int i = 0; i < w * h * 2; i += 2) {
        write_byte(color_p[i]);     // High byte
        write_byte(color_p[i + 1]); // Low byte
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
