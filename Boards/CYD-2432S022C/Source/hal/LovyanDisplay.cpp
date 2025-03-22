#include "LovyanDisplay.h"
#include "CYD2432S022CConstants.h"
#include "CST820Touch.h"
#include <esp_log.h>
#include <LovyanGFX.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>

static const char* TAG = "LovyanDisplay";

// Define a custom LovyanGFX class for the CYD-2432S022C board
class LGFX_CYD_2432S022C : public lgfx::LGFX_Device {
public:
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_Parallel8 _bus_instance;
    lgfx::Light_PWM _light_instance;

    LGFX_CYD_2432S022C() {
        ESP_LOGI(TAG, "Initializing LGFX_CYD_2432S022C display class");
        
        // Bus configuration
        {
            auto cfg = _bus_instance.config();
            cfg.pin_wr = CYD_2432S022C_LCD_PIN_WR;
            cfg.pin_rd = CYD_2432S022C_LCD_PIN_RD;
            cfg.pin_rs = CYD_2432S022C_LCD_PIN_DC;
            cfg.pin_d0 = CYD_2432S022C_LCD_PIN_D0;
            cfg.pin_d1 = CYD_2432S022C_LCD_PIN_D1;
            cfg.pin_d2 = CYD_2432S022C_LCD_PIN_D2;
            cfg.pin_d3 = CYD_2432S022C_LCD_PIN_D3;
            cfg.pin_d4 = CYD_2432S022C_LCD_PIN_D4;
            cfg.pin_d5 = CYD_2432S022C_LCD_PIN_D5;
            cfg.pin_d6 = CYD_2432S022C_LCD_PIN_D6;
            cfg.pin_d7 = CYD_2432S022C_LCD_PIN_D7;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // Panel configuration
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = CYD_2432S022C_LCD_PIN_CS;
            cfg.pin_rst = CYD_2432S022C_LCD_PIN_RST;
            cfg.pin_busy = -1;
            cfg.panel_width = CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION;
            cfg.panel_height = CYD_2432S022C_LCD_VERTICAL_RESOLUTION;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = true;
            cfg.invert = false;
            cfg.rgb_order = false;  // BGR order for correct colors
            cfg.dlen_16bit = false;
            cfg.bus_shared = true;
            _panel_instance.config(cfg);
        }

        // Backlight configuration
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl = GPIO_NUM_0;
            cfg.invert = false;
            cfg.freq = 44100;
            cfg.pwm_channel = 7;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        setPanel(&_panel_instance);
        ESP_LOGI(TAG, "LGFX_CYD_2432S022C initialization complete");
    }
};

// LovyanGFX display device implementation for Tactility
class LovyanGFXDisplay : public tt::hal::display::DisplayDevice {
public:
    struct Configuration {
        uint16_t width;
        uint16_t height;
        lv_display_rotation_t rotation;
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
        Configuration(uint16_t width, uint16_t height, lv_display_rotation_t rotation, std::shared_ptr<tt::hal::touch::TouchDevice> touch)
            : width(width), height(height), rotation(rotation), touch(std::move(touch)) {}
    };

    explicit LovyanGFXDisplay(std::unique_ptr<Configuration> config)
        : configuration(std::move(config)) {
        ESP_LOGI(TAG, "LovyanGFXDisplay constructor called with width=%d, height=%d, rotation=%d",
                 configuration->width, configuration->height, configuration->rotation);
    }

    ~LovyanGFXDisplay() override {
        stop();
    }

    bool start() override {
        if (isStarted) {
            ESP_LOGW(TAG, "Display already started");
            return true;
        }

        ESP_LOGI(TAG, "Starting LovyanGFX display");
        if (!lcd.init()) {
            ESP_LOGE(TAG, "Failed to initialize LovyanGFX display");
            return false;
        }
        lcd.writeCommand(0x11); // Sleep Out
        vTaskDelay(pdMS_TO_TICKS(120));
        lcd.writeCommand(0x3A); // Color Mode
        lcd.writeData(0x05);    // RGB565
        lcd.writeCommand(0x29); // Display ON
        vTaskDelay(pdMS_TO_TICKS(50));

        lcd.setBrightness(0);  // Start with backlight off

        ESP_LOGI(TAG, "Creating LVGL display: %dx%d", configuration->width, configuration->height);
        lvglDisplay = lv_display_create(configuration->width, configuration->height);
        if (!lvglDisplay) {
            ESP_LOGE(TAG, "Failed to create LVGL display");
            return false;
        }

        lv_display_set_color_format(lvglDisplay, LV_COLOR_FORMAT_RGB565);
        ESP_LOGI(TAG, "Setting initial display rotation to %d", configuration->rotation);
        lv_disp_set_rotation(lvglDisplay, configuration->rotation);

        // Calculate buffer size dynamically based on rotation
        const size_t buffer_width = (configuration->rotation == LV_DISPLAY_ROTATION_90 || configuration->rotation == LV_DISPLAY_ROTATION_270)
                                    ? configuration->height  // 320
                                    : configuration->width;  // 240
        const size_t buffer_height = 64;  // Partial rendering height
        bufferSize = buffer_width * buffer_height;  // e.g., 320 * 64 = 20480 pixels

        ESP_LOGI(TAG, "Heap free before buffer allocation: %d bytes (DMA: %d bytes)",
                 heap_caps_get_free_size(MALLOC_CAP_DEFAULT),
                 heap_caps_get_free_size(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
        buf1 = (uint16_t*)heap_caps_malloc(bufferSize * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        buf2 = (uint16_t*)heap_caps_malloc(bufferSize * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (!buf1 || !buf2) {
            ESP_LOGE(TAG, "Failed to allocate buffers! Size requested: %d bytes",
                     bufferSize * sizeof(uint16_t));
            return false;
        }
        ESP_LOGI(TAG, "Allocated buffers: buf1=%p, buf2=%p, size=%d bytes",
                 buf1, buf2, bufferSize * sizeof(uint16_t));

        // Set buffers with partial rendering
        ESP_LOGI(TAG, "Setting LVGL buffers with render mode PARTIAL");
        lv_display_set_buffers(lvglDisplay, buf1, buf2, bufferSize, LV_DISPLAY_RENDER_MODE_PARTIAL);
        ESP_LOGI(TAG, "LVGL buffers set successfully");

        // Set flush callback
        ESP_LOGI(TAG, "Setting flush callback");
        lv_display_set_flush_cb(lvglDisplay, [](lv_display_t* disp, const lv_area_t* area, uint8_t* data) {
            auto* display = static_cast<LovyanGFXDisplay*>(lv_display_get_user_data(disp));
            int32_t x1 = area->x1, y1 = area->y1, x2 = area->x2, y2 = area->y2;

            // Transform coordinates based on rotation
            lv_display_rotation_t current_rotation = lv_disp_get_rotation(disp);
            int32_t hw_x1, hw_y1, hw_x2, hw_y2;
            if (current_rotation == LV_DISPLAY_ROTATION_90) {
                hw_x1 = y1;
                hw_y1 = 320 - x2 - 1;
                hw_x2 = y2;
                hw_y2 = 320 - x1 - 1;
            } else if (current_rotation == LV_DISPLAY_ROTATION_270) {
                hw_x1 = 240 - y2 - 1;
                hw_y1 = x1;
                hw_x2 = 240 - y1 - 1;
                hw_y2 = x2;
            } else if (current_rotation == LV_DISPLAY_ROTATION_180) {
                hw_x1 = 240 - x2 - 1;
                hw_y1 = 320 - y2 - 1;
                hw_x2 = 240 - x1 - 1;
                hw_y2 = 320 - y1 - 1;
            } else {  // LV_DISPLAY_ROTATION_0
                hw_x1 = x1;
                hw_y1 = y1;
                hw_x2 = x2;
                hw_y2 = y2;
            }

            ESP_LOGI(TAG, "Flush callback: rotated x1=%" PRId32 ", y1=%" PRId32 ", x2=%" PRId32 ", y2=%" PRId32
                          " -> hw_x1=%" PRId32 ", hw_y1=%" PRId32 ", hw_x2=%" PRId32 ", hw_y2=%" PRId32,
                     x1, y1, x2, y2, hw_x1, hw_y1, hw_x2, hw_y2);

            display->lcd.startWrite();
            display->lcd.setAddrWindow(hw_x1, hw_y1, hw_x2 - hw_x1 + 1, hw_y2 - hw_y1 + 1);
            display->lcd.writePixels((lgfx::rgb565_t*)data, (x2 - x1 + 1) * (y2 - y1 + 1));
            display->lcd.endWrite();
            lv_display_flush_ready(disp);
        });

        isStarted = true;
        ESP_LOGI(TAG, "LovyanGFX display started successfully");
        return true;
    }

    bool stop() override {
        if (!isStarted) return true;
        if (configuration && configuration->touch) configuration->touch->stop();
        if (lvglDisplay) lv_display_delete(lvglDisplay);
        lvglDisplay = nullptr;
        if (buf1) heap_caps_free(buf1);
        if (buf2) heap_caps_free(buf2);
        buf1 = nullptr;
        buf2 = nullptr;
        isStarted = false;
        return true;
    }

    void setBacklightDuty(uint8_t backlightDuty) override {
        if (isStarted) lcd.setBrightness(backlightDuty);
    }

    bool supportsBacklightDuty() const override { return true; }

    lv_display_t* getLvglDisplay() const override { return lvglDisplay; }

    std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() override {
        return configuration ? configuration->touch : nullptr;
    }

    std::string getName() const override { return "CYD-2432S022C Display"; }
    std::string getDescription() const override { return "LovyanGFX-based display for CYD-2432S022C board"; }

private:
    std::unique_ptr<Configuration> configuration;
    LGFX_CYD_2432S022C lcd;
    lv_display_t* lvglDisplay = nullptr;
    bool isStarted = false;
    uint16_t* buf1 = nullptr;
    uint16_t* buf2 = nullptr;
    size_t bufferSize = 0;
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createCST820Touch();
    if (!touch) {
        ESP_LOGE(TAG, "Failed to create touch device!");
        return nullptr;
    }

    // Get the Tactility orientation dynamically
    lv_display_rotation_t tactility_orientation = tt::app::display::getRotation();
    ESP_LOGI(TAG, "Creating display with Tactility orientation: %d", tactility_orientation);

    auto config = std::make_unique<LovyanGFXDisplay::Configuration>(
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,  // 240
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION,    // 320
        tactility_orientation,                     // Use Tactility orientation
        touch
    );
    return std::make_shared<LovyanGFXDisplay>(std::move(config));
}
