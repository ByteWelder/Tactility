#include "LovyanDisplay.h"
#include "CYD2432S022CConstants.h"
#include "CST820Touch.h"
#include <PwmBacklight.h>
#include <esp_log.h>
#include <LovyanGFX.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
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
            ESP_LOGI(TAG, "Entering bus configuration block");
            auto cfg = _bus_instance.config();
            ESP_LOGI(TAG, "Configuring LCD bus pins: WR=%d, RD=%d, DC=%d, D0-D7=%d,%d,%d,%d,%d,%d,%d,%d",
                     CYD_2432S022C_LCD_PIN_WR, CYD_2432S022C_LCD_PIN_RD, CYD_2432S022C_LCD_PIN_DC,
                     CYD_2432S022C_LCD_PIN_D0, CYD_2432S022C_LCD_PIN_D1, CYD_2432S022C_LCD_PIN_D2,
                     CYD_2432S022C_LCD_PIN_D3, CYD_2432S022C_LCD_PIN_D4, CYD_2432S022C_LCD_PIN_D5,
                     CYD_2432S022C_LCD_PIN_D6, CYD_2432S022C_LCD_PIN_D7);
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
            ESP_LOGI(TAG, "LCD bus configuration applied successfully");
        }

        // Panel configuration
        {
            ESP_LOGI(TAG, "Entering panel configuration block");
            auto cfg = _panel_instance.config();
            ESP_LOGI(TAG, "Configuring LCD panel: CS=%d, RST=%d, W=%d, H=%d",
                     CYD_2432S022C_LCD_PIN_CS, CYD_2432S022C_LCD_PIN_RST,
                     CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION, CYD_2432S022C_LCD_VERTICAL_RESOLUTION);
            cfg.pin_cs = CYD_2432S022C_LCD_PIN_CS;
            cfg.pin_rst = CYD_2432S022C_LCD_PIN_RST;
            cfg.pin_busy = -1;
            cfg.panel_width = CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION;  // 240
            cfg.panel_height = CYD_2432S022C_LCD_VERTICAL_RESOLUTION;   // 320
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = true;
            cfg.invert = false;
            cfg.rgb_order = true;  // RGB order
            cfg.dlen_16bit = false;
            cfg.bus_shared = true;
            _panel_instance.config(cfg);
            ESP_LOGI(TAG, "LCD panel configuration applied successfully");
        }

        // Backlight configuration
        {
            ESP_LOGI(TAG, "Entering backlight configuration block");
            auto cfg = _light_instance.config();
            ESP_LOGI(TAG, "Configuring backlight: pin=%d, freq=%d, pwm_channel=%d",
                     CYD_2432S022C_LCD_PIN_BACKLIGHT, 44100, 7);
            cfg.pin_bl = CYD_2432S022C_LCD_PIN_BACKLIGHT;
            cfg.invert = false;
            cfg.freq = 44100;
            cfg.pwm_channel = 7;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
            ESP_LOGI(TAG, "Backlight configured successfully");
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
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
        Configuration(uint16_t width, uint16_t height, std::shared_ptr<tt::hal::touch::TouchDevice> touch)
            : width(width), height(height), touch(std::move(touch)) {}
    };

    explicit LovyanGFXDisplay(std::unique_ptr<Configuration> config)
        : configuration(std::move(config)) {
        ESP_LOGI(TAG, "LovyanGFXDisplay constructor called with width=%d, height=%d",
                 configuration->width, configuration->height);
    }

    ~LovyanGFXDisplay() override {
        stop();
    }

    bool start() override {
        if (isStarted) {
            ESP_LOGW(TAG, "Display already started");
            return true;
        }

        if (!configuration) {
            ESP_LOGE(TAG, "Cannot start: configuration is null!");
            return false;
        }

        ESP_LOGI(TAG, "Disabling task watchdog for initialization");
        esp_task_wdt_delete(xTaskGetCurrentTaskHandle());
        esp_task_wdt_deinit();

        ESP_LOGI(TAG, "Starting LovyanGFX display");
        lcd.init();
        lcd.writeCommand(0x11); // Sleep Out
        vTaskDelay(pdMS_TO_TICKS(120));
        lcd.writeCommand(0x3A); // Color Mode
        lcd.writeData(0x05);    // RGB565
        lcd.writeCommand(0x29); // Display ON
        vTaskDelay(pdMS_TO_TICKS(50));

        ESP_LOGI(TAG, "Setting brightness to 128");
        lcd.setBrightness(128);
        lcd.setRotation(0);

        // Test fill: Red screen
        ESP_LOGI(TAG, "Filling screen with red to test display");
        lcd.fillScreen(lcd.color565(255, 0, 0));
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1s to observe

        ESP_LOGI(TAG, "Creating LVGL display: %dx%d",
                 configuration->width, configuration->height);
        lvglDisplay = lv_display_create(configuration->width, configuration->height);
        if (!lvglDisplay) {
            ESP_LOGE(TAG, "Failed to create LVGL display");
            return false;
        }

        lv_display_set_color_format(lvglDisplay, LV_COLOR_FORMAT_RGB565);

        ESP_LOGI(TAG, "Heap free before buffer allocation: %d bytes (DMA: %d bytes)",
                 heap_caps_get_free_size(MALLOC_CAP_DEFAULT),
                 heap_caps_get_free_size(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
        static uint16_t* buf1 = nullptr;
        static uint16_t* buf2 = nullptr;
        buf1 = (uint16_t*)heap_caps_malloc(CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        buf2 = (uint16_t*)heap_caps_malloc(CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (!buf1 || !buf2) {
            ESP_LOGE(TAG, "Failed to allocate buffers! Size requested: %d bytes",
                     CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t));
            return false;
        }
        ESP_LOGI(TAG, "Allocated buffers: buf1=%p, buf2=%p, size=%d bytes",
                 buf1, buf2, CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t));

        ESP_LOGI(TAG, "Setting LVGL buffers with render mode FULL");
        lv_display_set_buffers(lvglDisplay, buf1, buf2, CYD_2432S022C_LCD_DRAW_BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_FULL);
        ESP_LOGI(TAG, "LVGL buffers set successfully");

        ESP_LOGI(TAG, "Setting flush callback");
        lv_display_set_flush_cb(lvglDisplay, [](lv_display_t* disp, const lv_area_t* area, uint8_t* data) {
            auto* display = static_cast<LovyanGFXDisplay*>(lv_display_get_user_data(disp));
            ESP_LOGI(TAG, "Flush callback: x1=%" PRId32 ", y1=%" PRId32 ", x2=%" PRId32 ", y2=%" PRId32,
                     area->x1, area->y1, area->x2, area->y2);
            display->lcd.startWrite();
            display->lcd.setAddrWindow(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
            display->lcd.writePixels((lgfx::rgb565_t*)data, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1));
            vTaskDelay(pdMS_TO_TICKS(1));
            display->lcd.endWrite();
            lv_display_flush_ready(disp);
        });

        lv_display_set_user_data(lvglDisplay, this);

        if (configuration->touch) {
            configuration->touch->start(lvglDisplay);
            ESP_LOGI(TAG, "Touch input started");
        }

        ESP_LOGI(TAG, "Re-enabling task watchdog");
        esp_task_wdt_config_t wdt_config = {
            .timeout_ms = 20000,
            .idle_core_mask = 0,
            .trigger_panic = true
        };
        esp_err_t err = esp_task_wdt_init(&wdt_config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize watchdog: %s", esp_err_to_name(err));
        }
        esp_task_wdt_add(xTaskGetCurrentTaskHandle());

        isStarted = true;
        ESP_LOGI(TAG, "LovyanGFX display started successfully");
        return true;
    }

    bool stop() override {
        if (!isStarted) return true;
        if (configuration && configuration->touch) configuration->touch->stop();
        if (lvglDisplay) lv_display_delete(lvglDisplay);
        lvglDisplay = nullptr;
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
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createCST820Touch();
    if (!touch) {
        ESP_LOGE(TAG, "Failed to create touch device!");
        return nullptr;
    }

    auto config = std::make_unique<LovyanGFXDisplay::Configuration>(
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION,
        touch
    );
    return std::make_shared<LovyanGFXDisplay>(std::move(config));
}
