#include "ST7789Display.h"
#include "CST820Touch.h"
#include "CYD2432S022CConstants.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <inttypes.h>

static const char *TAG = "ST7789Display";

ST7789Display::ST7789Display(std::unique_ptr<Configuration> config)
    : config_(std::move(config)) {}

bool ST7789Display::initialize_hardware() {
    ESP_LOGI(TAG, "Initializing ST7789 display hardware...");

    // Configure GPIO pins for the display
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

    // Configure backlight pin (GPIO 0, as per OpenHASP configs)
    #define BACKLIGHT_PIN GPIO_NUM_0
    ESP_LOGI(TAG, "Configuring backlight pin (GPIO %d)", BACKLIGHT_PIN);
    gpio_config_t backlight_conf = {
        .pin_bit_mask = (1ULL << BACKLIGHT_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    if (gpio_config(&backlight_conf) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure backlight pin");
        return false;
    }
    gpio_set_level(BACKLIGHT_PIN, 1); // Turn on backlight
    ESP_LOGI(TAG, "Backlight turned on");

    // ST7789 initialization sequence
    ESP_LOGI(TAG, "Starting ST7789 display initialization");
    gpio_set_level(CYD_2432S022C_LCD_PIN_CS, 0);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // Software reset
    ESP_LOGI(TAG, "Sending software reset (SWRESET)");
    write_byte(0x01);
    vTaskDelay(pdMS_TO_TICKS(200)); // Increased delay as per online research

    // Sleep out
    ESP_LOGI(TAG, "Sending sleep out (SLPOUT)");
    write_byte(0x11);
    vTaskDelay(pdMS_TO_TICKS(200)); // Increased delay

    // Frame rate control (FRMCTR1)
    ESP_LOGI(TAG, "Setting frame rate control (FRMCTR1)");
    write_byte(0xB1);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    write_byte(0x05); // Frame rate control parameters (adjust as needed)
    write_byte(0x3C);
    write_byte(0x3C);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // Frame rate control (FRMCTR2)
    ESP_LOGI(TAG, "Setting frame rate control (FRMCTR2)");
    write_byte(0xB2);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    write_byte(0x05);
    write_byte(0x3C);
    write_byte(0x3C);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // Porch control (PORCTRL)
    ESP_LOGI(TAG, "Setting porch control (PORCTRL)");
    write_byte(0xB2);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    write_byte(0x0C);
    write_byte(0x0C);
    write_byte(0x00);
    write_byte(0x33);
    write_byte(0x33);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // Gate control (GCTRL)
    ESP_LOGI(TAG, "Setting gate control (GCTRL)");
    write_byte(0xB7);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    write_byte(0x35);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // VCOM setting (VCOMS)
    ESP_LOGI(TAG, "Setting VCOM (VCOMS)");
    write_byte(0xBB);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    write_byte(0x20);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // LCM control (LCMCTRL)
    ESP_LOGI(TAG, "Setting LCM control (LCMCTRL)");
    write_byte(0xC0);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    write_byte(0x2C);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // VDV/VRH command enable (VDVVRHEN)
    ESP_LOGI(TAG, "Setting VDV/VRH command enable (VDVVRHEN)");
    write_byte(0xC2);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    write_byte(0x01);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // VRH set (VRHS)
    ESP_LOGI(TAG, "Setting VRH (VRHS)");
    write_byte(0xC3);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    write_byte(0x12);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // VDV set (VDVS)
    ESP_LOGI(TAG, "Setting VDV (VDVS)");
    write_byte(0xC4);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    write_byte(0x20);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // Frame rate control in idle mode (FRCTRL2)
    ESP_LOGI(TAG, "Setting frame rate control in idle mode (FRCTRL2)");
    write_byte(0xC6);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    write_byte(0x0F);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // Power control 1 (PWCTRL1)
    ESP_LOGI(TAG, "Setting power control 1 (PWCTRL1)");
    write_byte(0xD0);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    write_byte(0xA4);
    write_byte(0xA1);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // Color mode: 16-bit RGB565
    ESP_LOGI(TAG, "Setting color mode to RGB565 (COLMOD)");
    write_byte(0x3A);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    write_byte(0x55); // RGB565
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // Memory access control (orientation and RGB/BGR order)
    ESP_LOGI(TAG, "Setting memory access control (MADCTL)");
    write_byte(0x36);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 1); // Data mode
    uint8_t madctl = 0x00; // Default orientation (worked in LovyanGFX)
    if (!config_->rgb_order) { // If false, use BGR order
        madctl |= 0x08; // Set RGB/BGR bit to 1 for BGR order
        ESP_LOGI(TAG, "Using BGR order (MADCTL bit 3 set)");
    } else {
        ESP_LOGI(TAG, "Using RGB order (MADCTL bit 3 clear)");
    }
    write_byte(madctl);
    gpio_set_level(CYD_2432S022C_LCD_PIN_RS, 0); // Command mode

    // Display inversion
    if (config_->invert) {
        ESP_LOGI(TAG, "Enabling display inversion (INVON)");
        write_byte(0x21); // Inversion on
    } else {
        ESP_LOGI(TAG, "Disabling display inversion (INVOFF)");
        write_byte(0x20); // Inversion off
    }

    // Normal display mode on
    ESP_LOGI(TAG, "Setting normal display mode (NORON)");
    write_byte(0x13);

    // Display on
    ESP_LOGI(TAG, "Turning display on (DISPON)");
    write_byte(0x29);
    ESP_LOGI(TAG, "Display on, delaying 50ms");
    vTaskDelay(pdMS_TO_TICKS(50)); // Match LovyanGFX delay

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

        // Set up the draw buffer (increased to 20 rows)
        static lv_color_t buf[CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION * 20]; // Buffer for 20 rows
        lv_display_set_buffers(display_, buf, nullptr, CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION * 20, LV_DISPLAY_RENDER_MODE_PARTIAL);

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

    ESP_LOGI(TAG, "Flushing area x1=%" PRId32 ", y1=%" PRId32 ", x2=%" PRId32 ", y2=%" PRId32,
             area->x1, area->y1, area->x2, area->y2);

    int w = (area->x2 - area->x1 + 1);
    int h = (area->y2 - area->y1 + 1);

    set_address_window(area->x1, area->y1, w, h);

    ESP_LOGI(TAG, "Writing %d pixels (w=%d, h=%d)", (int)(w * h), w, h);
    for (int i = 0; i < w * h; i++) {
        uint16_t rgb565 = 0x07E0; // Green in RGB565: R=0x00, G=0x3F, B=0x00
        uint8_t high = (rgb565 >> 8) & 0xFF; // 0x07
        uint8_t low = rgb565 & 0xFF;         // 0xE0

        if (i < 4) {
            ESP_LOGI(TAG, "Pixel %d: RGB565 high=%02x, low=%02x", i, high, low);
        }

        write_byte(low);
        write_byte(high);
    }

    gpio_set_level(CYD_2432S022C_LCD_PIN_CS, 1);
    lv_display_flush_ready(display_);
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createCST820Touch();

    auto configuration = std::make_unique<ST7789Display::Configuration>(
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION,
        touch,
        false, // invert: false (inversion off, as tested)
        true   // rgb_order: true (RGB order)
    );

    return std::make_shared<ST7789Display>(std::move(configuration));
}
