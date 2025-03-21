#include "LovyanDisplay.h"
#include "CYD2432S022CConstants.h"
#include "CST820Touch.h"
#include <PwmBacklight.h>
#include <esp_log.h>
#include <LovyanGFX.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h> // For vTaskDelay
#include <freertos/task.h>     // For pdMS_TO_TICKS
#include <esp_task_wdt.h>      // For watchdog control
#include <esp_heap_caps.h>     // For heap logging

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
            ESP_LOGI(TAG, "Bus config applied, setting bus to panel");
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
            cfg.rgb_order = true;  // BGR
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
            ESP_LOGI(TAG, "Backlight config applied, setting to panel");
            _panel_instance.setLight(&_light_instance);
            ESP_LOGI(TAG, "Backlight configured successfully");
        }

        ESP_LOGI(TAG, "Setting panel instance to device");
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
        ESP_LOGI(TAG, "LovyanGFXDisplay constructor entered, configuration=%p", configuration.get());
        if (!configuration) {
            ESP_LOGE(TAG, "Configuration is null! Aborting");
            return;
        }
        ESP_LOGI(TAG, "LovyanGFXDisplay constructor called with width=%d, height=%d",
                 configuration->width, configuration->height);
    }

    ~LovyanGFXDisplay() override {
        ESP_LOGI(TAG, "Destructor called: Stopping LovyanGFX display");
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
        ESP_LOGI(TAG, "Calling lcd.init()");
        lcd.init();
        // Additional ST7789 initialization commands
        ESP_LOGI(TAG, "Sending ST7789 initialization commands");
        lcd.writeCommand(0x11); // Sleep Out (SLPOUT)
        vTaskDelay(pdMS_TO_TICKS(120)); // Wait for sleep out
        lcd.writeCommand(0x3A); // Color Mode (COLMOD)
        lcd.writeData(0x05); // 16-bit/pixel (RGB565)
        lcd.writeCommand(0x29); // Display ON (DISPON)
        ESP_LOGI(TAG, "LCD init called, delaying 50ms");
        vTaskDelay(pdMS_TO_TICKS(50));
        ESP_LOGI(TAG, "Yielding to system after lcd.init");
        vTaskDelay(pdMS_TO_TICKS(10));

        ESP_LOGI(TAG, "Setting brightness to 128");
        lcd.setBrightness(128);
        ESP_LOGI(TAG, "Backlight on, screen should be visible");
        ESP_LOGI(TAG, "Setting rotation to 0");
        lcd.setRotation(0);
        ESP_LOGI(TAG, "Display initialized with brightness=128 and rotation=1");

        ESP_LOGI(TAG, "Creating LVGL display: %dx%d",
                 CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION, CYD_2432S022C_LCD_VERTICAL_RESOLUTION);
        lvglDisplay = lv_display_create(CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,
                                        CYD_2432S022C_LCD_VERTICAL_RESOLUTION);  // 240x320
        ESP_LOGI(TAG, "LVGL display create called");
        vTaskDelay(pdMS_TO_TICKS(10));
        if (!lvglDisplay) {
            ESP_LOGE(TAG, "Failed to create LVGL display");
            return false;
        }
        ESP_LOGI(TAG, "LVGL display created: %dx%d", CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,
                 CYD_2432S022C_LCD_VERTICAL_RESOLUTION);

        ESP_LOGI(TAG, "Setting color format to RGB565");
        lv_display_set_color_format(lvglDisplay, LV_COLOR_FORMAT_RGB565);

        // Hardcode buffer allocation using uint16_t for RGB565 (2 bytes per pixel)
        static uint16_t* buf1 = nullptr;
        static uint16_t* buf2 = nullptr;
        ESP_LOGI(TAG, "Heap free before buffer allocation: %d bytes (DMA: %d bytes)",
                 heap_caps_get_free_size(MALLOC_CAP_DEFAULT),
                 heap_caps_get_free_size(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
        buf1 = (uint16_t*)heap_caps_malloc(CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        buf2 = (uint16_t*)heap_caps_malloc(CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (!buf1 || !buf2) {
            ESP_LOGE(TAG, "Failed to allocate buffers!");
            return false;
        }
        ESP_LOGI(TAG, "Allocated buffers: buf1=%p, buf2=%p, sizeof(uint16_t)=%d", buf1, buf2, sizeof(uint16_t));
        ESP_LOGI(TAG, "Buffer alignment check: buf1 mod 4=%d, buf2 mod 4=%d", (uintptr_t)buf1 % 4, (uintptr_t)buf2 % 4);
        ESP_LOGI(TAG, "Heap free after buffer allocation: %d bytes (DMA: %d bytes)",
                 heap_caps_get_free_size(MALLOC_CAP_DEFAULT),
                 heap_caps_get_free_size(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
        ESP_LOGI(TAG, "Yielding before setting LVGL buffers");
        vTaskDelay(pdMS_TO_TICKS(10));

        ESP_LOGI(TAG, "Setting LVGL buffers with render mode FULL");
        vTaskDelay(pdMS_TO_TICKS(10)); // Yield before setting buffers
        lv_display_set_buffers(lvglDisplay, buf1, buf2, CYD_2432S022C_LCD_DRAW_BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_FULL);
        ESP_LOGI(TAG, "LVGL buffers set successfully");
        vTaskDelay(pdMS_TO_TICKS(10)); // Yield after setting buffers
        ESP_LOGI(TAG, "LVGL buffers set, yielding to system");
        vTaskDelay(pdMS_TO_TICKS(10));

        ESP_LOGI(TAG, "Setting flush callback");
        lv_display_set_flush_cb(lvglDisplay, [](lv_display_t* disp, const lv_area_t* area, uint8_t* data) {
            ESP_LOGI(TAG, "Flush callback entered: area=(%" PRId32 ", %" PRId32 ") to (%" PRId32 ", %" PRId32 ")",
                     area->x1, area->y1, area->x2, area->y2);
            auto* display = static_cast<LovyanGFXDisplay*>(lv_display_get_user_data(disp));
            ESP_LOGI(TAG, "Starting write transaction");
            display->lcd.startWrite();
            ESP_LOGI(TAG, "Setting address window: x=%" PRId32 ", y=%" PRId32 ", w=%" PRId32 ", h=%" PRId32,
                     area->x1, area->y1, (area->x2 - area->x1 + 1), (area->y2 - area->y1 + 1));
            display->lcd.setAddrWindow(area->x1, area->y1, (area->x2 - area->x1 + 1), (area->y2 - area->y1 + 1));
            ESP_LOGI(TAG, "Writing pixels: len=%" PRId32, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1));
            display->lcd.writePixels((lgfx::rgb565_t*)data, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1));
            ESP_LOGI(TAG, "Yielding during flush to avoid watchdog timeout");
            vTaskDelay(pdMS_TO_TICKS(1));
            ESP_LOGI(TAG, "Ending write transaction");
            display->lcd.endWrite();
            ESP_LOGI(TAG, "Notifying LVGL flush ready");
            lv_display_flush_ready(disp);
            ESP_LOGI(TAG, "Flush callback completed");
        });
        vTaskDelay(pdMS_TO_TICKS(10));

        ESP_LOGI(TAG, "Setting user data to this=%p", this);
        lv_display_set_user_data(lvglDisplay, this);

        if (configuration->touch) {
            ESP_LOGI(TAG, "Starting touch input");
            configuration->touch->start(lvglDisplay);
            ESP_LOGI(TAG, "Touch input started");
        }

        ESP_LOGI(TAG, "Re-enabling task watchdog");
        esp_task_wdt_config_t wdt_config = {
            .timeout_ms = 20000, // 20 seconds timeout
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
        ESP_LOGI(TAG, "Stopping LovyanGFX display");
        if (!isStarted) {
            ESP_LOGW(TAG, "Display already stopped");
            return true;
        }
        if (configuration && configuration->touch) {
            ESP_LOGI(TAG, "Stopping touch");
            configuration->touch->stop();
            ESP_LOGI(TAG, "Touch stopped");
        }
        if (lvglDisplay) {
            ESP_LOGI(TAG, "Deleting LVGL display");
            lv_display_delete(lvglDisplay);
            lvglDisplay = nullptr;
            ESP_LOGI(TAG, "LVGL display deleted");
        }
        isStarted = false;
        ESP_LOGI(TAG, "LovyanGFX display stopped");
        return true;
    }

    void setBacklightDuty(uint8_t backlightDuty) override {
        ESP_LOGI(TAG, "Setting backlight duty to %d", backlightDuty);
        if (isStarted) {
            lcd.setBrightness(backlightDuty);
            ESP_LOGI(TAG, "Backlight duty set");
        }
    }

    bool supportsBacklightDuty() const override {
        ESP_LOGI(TAG, "Checking backlight duty support: returning true");
        return true;
    }

    lv_display_t* getLvglDisplay() const override {
        ESP_LOGI(TAG, "Getting LVGL display: %p", lvglDisplay);
        return lvglDisplay;
    }

    std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() override {
        ESP_LOGI(TAG, "Creating touch device from configuration");
        return configuration ? configuration->touch : nullptr;
    }

    std::string getName() const override {
        ESP_LOGI(TAG, "Returning display name: CYD-2432S022C Display");
        return "CYD-2432S022C Display";
    }

    std::string getDescription() const override {
        ESP_LOGI(TAG, "Returning display description: LovyanGFX-based display for CYD-2432S022C board");
        return "LovyanGFX-based display for CYD-2432S022C board";
    }

private:
    std::unique_ptr<Configuration> configuration;
    LGFX_CYD_2432S022C lcd;
    lv_display_t* lvglDisplay = nullptr;
    bool isStarted = false;
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    ESP_LOGI(TAG, "Creating display");
    auto touch = tt::hal::touch::createTouch();
    ESP_LOGI(TAG, "Touch created: %p", touch.get());
    if (!touch) {
        ESP_LOGE(TAG, "Failed to create touch device!");
        return nullptr;
    }

    // Create configuration
    auto config = new LovyanGFXDisplay::Configuration(
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION,
        touch
    );
    auto configuration = std::unique_ptr<LovyanGFXDisplay::Configuration>(config);
    ESP_LOGI(TAG, "Display configuration created: %p, width=%d, height=%d",
             configuration.get(), CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION, CYD_2432S022C_LCD_VERTICAL_RESOLUTION);
    if (!configuration) {
        ESP_LOGE(TAG, "Failed to create configuration!");
        return nullptr;
    }

    // Create display with configuration
    ESP_LOGI(TAG, "Creating LovyanGFXDisplay with configuration=%p", configuration.get());
    auto display = std::make_shared<LovyanGFXDisplay>(std::move(configuration));
    if (!display) {
        ESP_LOGE(TAG, "Failed to create display!");
        return nullptr;
    }

    // Double-check configuration after move
    ESP_LOGI(TAG, "Verifying configuration after move");
    auto created_display = display.get();
    auto config_after_move = created_display->createTouch(); // Indirectly checks configuration
    ESP_LOGI(TAG, "Configuration touch after move: %p", config_after_move.get());

    ESP_LOGI(TAG, "Returning new LovyanGFXDisplay: %p", display.get());
    return display;
}
