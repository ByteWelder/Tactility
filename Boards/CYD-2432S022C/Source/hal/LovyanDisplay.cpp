#include "LovyanDisplay.h"
#include "CYD2432S022CConstants.h"
#include "CST820Touch.h"
#include "Tactility/app/display/DisplaySettings.h"
#include <driver/gpio.h>  // Added for backlight pin configuration
#include <esp_log.h>
#include <LovyanGFX.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>

static const char* TAG = "LovyanDisplay";

// Hardware resolution
#define HARDWARE_WIDTH 240
#define HARDWARE_HEIGHT 320

// Default orientation if not set in NVS
#define DEFAULT_ORIENTATION LV_DISPLAY_ROTATION_0  // Portrait, USB at bottom

// cyd22_init() function
static bool cyd22_init() {
    ESP_LOGI(TAG, "Running cyd22_init");

    // Step 1: Configure backlight pin (LovyanGFX will handle PWM later)
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_0);  // Backlight pin
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure backlight pin");
        return false;
    }
    if (gpio_set_level(GPIO_NUM_0, 0) != ESP_OK) return false;  // Backlight off initially

    // Step 2: Get Tactility orientation and set default if needed
    lv_display_rotation_t tactility_orientation = tt::app::display::getRotation();
    if (tactility_orientation == LV_DISPLAY_ROTATION_0) {
        ESP_LOGI(TAG, "No orientation set in NVS. Setting default orientation: %d", DEFAULT_ORIENTATION);
        tt::app::display::setRotation(DEFAULT_ORIENTATION);
        tactility_orientation = DEFAULT_ORIENTATION;
    } else {
        ESP_LOGI(TAG, "Tactility orientation from NVS: %d", tactility_orientation);
    }

    return true;
}

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
            cfg.pin_rst = -1;  // No reset pin
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

        // Call cyd22_init() at the very beginning
        if (!cyd22_init()) {
            ESP_LOGE(TAG, "Failed to run cyd22_init");
            return false;
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

        // Set rotation in LovyanGFX to match LVGL
        uint8_t lovyan_rotation = 0;
        switch (configuration->rotation) {
            case LV_DISPLAY_ROTATION_0:   lovyan_rotation = 1; break;  // Portrait, USB at bottom
            case LV_DISPLAY_ROTATION_90:  lovyan_rotation = 0; break;  // Landscape, USB on right
            case LV_DISPLAY_ROTATION_180: lovyan_rotation = 3; break;  // Portrait, USB at top
            case LV_DISPLAY_ROTATION_270: lovyan_rotation = 2; break;  // Landscape, USB on left
        }
        lcd.setRotation(lovyan_rotation);
        ESP_LOGI(TAG, "Set LovyanGFX rotation to %d", lovyan_rotation);

        lcd.setBrightness(0);  // Start with backlight off

        // Calculate buffer size dynamically based on rotation
        const size_t buffer_width = (configuration->rotation == LV_DISPLAY_ROTATION_90 || configuration->rotation == LV_DISPLAY_ROTATION_270)
                                    ? configuration->height  // 320
                                    : configuration->width;  // 240
        const size_t buffer_height = 64;  // Partial rendering height
        bufferSize = buffer_width * buffer_height;  // e.g., 320 * 64 = 20480 pixels

        // Allocate and check buffers first
        ESP_LOGI(TAG, "Heap free before buffer allocation: %d bytes (DMA: %d bytes)",
                 heap_caps_get_free_size(MALLOC_CAP_DEFAULT),
                 heap_caps_get_free_size(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
        buf1 = (uint16_t*)heap_caps_malloc(bufferSize * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        buf2 = (uint16_t*)heap_caps_malloc(bufferSize * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (!buf1 || !buf2) {
            ESP_LOGE(TAG, "Failed to allocate buffers! Size requested: %d bytes",
                     bufferSize * sizeof(uint16_t));
            if (buf1) heap_caps_free(buf1);
            if (buf2) heap_caps_free(buf2);
            buf1 = nullptr;
            buf2 = nullptr;
            return false;
        }
        ESP_LOGI(TAG, "Allocated buffers: buf1=%p, buf2=%p, size=%d bytes",
                 buf1, buf2, bufferSize * sizeof(uint16_t));

        // Check buffers before creating display
        if (!buf1 || !buf2 || bufferSize == 0) {
            ESP_LOGE(TAG, "Invalid buffers: buf1=%p, buf2=%p, bufferSize=%d",
                     buf1, buf2, bufferSize);
            if (buf1) heap_caps_free(buf1);
            if (buf2) heap_caps_free(buf2);
            buf1 = nullptr;
            buf2 = nullptr;
            return false;
        }

        // Now create the LVGL display
        ESP_LOGI(TAG, "Creating LVGL display: %dx%d", configuration->width, configuration->height);
        lvglDisplay = lv_display_create(configuration->width, configuration->height);
        if (!lvglDisplay) {
            ESP_LOGE(TAG, "Failed to create LVGL display");
            if (buf1) heap_caps_free(buf1);
            if (buf2) heap_caps_free(buf2);
            buf1 = nullptr;
            buf2 = nullptr;
            return false;
        }

        lv_display_set_color_format(lvglDisplay, LV_COLOR_FORMAT_RGB565);
        ESP_LOGI(TAG, "Setting initial display rotation to %d", configuration->rotation);
        lv_disp_set_rotation(lvglDisplay, configuration->rotation);

        // Set buffers with partial rendering
        ESP_LOGI(TAG, "Setting LVGL buffers with render mode PARTIAL");
        lv_display_set_buffers(lvglDisplay, buf1, buf2, bufferSize, LV_DISPLAY_RENDER_MODE_PARTIAL);
        ESP_LOGI(TAG, "LVGL buffers set successfully");

        // Check display and buffers before setting flush callback
        if (!lvglDisplay || !buf1 || !buf2) {
            ESP_LOGE(TAG, "Cannot set flush callback: display=%p, buf1=%p, buf2=%p",
                     lvglDisplay, buf1, buf2);
            if (buf1) heap_caps_free(buf1);
            if (buf2) heap_caps_free(buf2);
            buf1 = nullptr;
            buf2 = nullptr;
            return false;
        }

        // Set flush callback
        ESP_LOGI(TAG, "Setting flush callback");
        lv_display_set_flush_cb(lvglDisplay, [](lv_display_t* disp, const lv_area_t* area, uint8_t* data) {
            // Since Tactility sets the user data, retrieve it
            auto* display = static_cast<LovyanGFXDisplay*>(lv_display_get_user_data(disp));
            if (!display) {
                ESP_LOGE(TAG, "Flush callback: Display pointer is null!");
                lv_display_flush_ready(disp);
                return;
            }
            if (!data) {
                ESP_LOGE(TAG, "Flush callback: Data pointer is null!");
                lv_display_flush_ready(disp);
                return;
            }

            int32_t x1 = area->x1, y1 = area->y1, x2 = area->x2, y2 = area->y2;
            int32_t width = x2 - x1 + 1;
            int32_t height = y2 - y1 + 1;

            ESP_LOGI(TAG, "Flush callback: data=%p, x1=%" PRId32 ", y1=%" PRId32 ", x2=%" PRId32 ", y2=%" PRId32
                          ", writing %" PRId32 " pixels",
                     data, x1, y1, x2, y2, width * height);

            // Since LovyanGFX handles rotation, use the logical coordinates directly
            display->lcd.startWrite();
            display->lcd.setAddrWindow(x1, y1, width, height);
            display->lcd.writePixels((lgfx::rgb565_t*)data, width * height);
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
