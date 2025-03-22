#include "LovyanDisplay.h"
#include "CYD2432S022CConstants.h"
#include "CST820Touch.h"
#include <LovyanGFX.h>
#include <esp_log.h>

static const char* TAG = "LovyanDisplay";

class LGFX_CYD_2432S022C : public lgfx::LGFX_Device {
public:
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_Parallel8 _bus_instance;
    lgfx::Light_PWM _light_instance;

    LGFX_CYD_2432S022C() {
        auto bus_cfg = _bus_instance.config();
        bus_cfg.pin_wr = CYD_2432S022C_LCD_PIN_WR;
        bus_cfg.pin_rd = CYD_2432S022C_LCD_PIN_RD;
        bus_cfg.pin_rs = CYD_2432S022C_LCD_PIN_DC;
        bus_cfg.pin_d0 = CYD_2432S022C_LCD_PIN_D0;
        bus_cfg.pin_d1 = CYD_2432S022C_LCD_PIN_D1;
        bus_cfg.pin_d2 = CYD_2432S022C_LCD_PIN_D2;
        bus_cfg.pin_d3 = CYD_2432S022C_LCD_PIN_D3;
        bus_cfg.pin_d4 = CYD_2432S022C_LCD_PIN_D4;
        bus_cfg.pin_d5 = CYD_2432S022C_LCD_PIN_D5;
        bus_cfg.pin_d6 = CYD_2432S022C_LCD_PIN_D6;
        bus_cfg.pin_d7 = CYD_2432S022C_LCD_PIN_D7;
        _bus_instance.config(bus_cfg);
        _panel_instance.setBus(&_bus_instance);

        auto panel_cfg = _panel_instance.config();
        panel_cfg.pin_cs = CYD_2432S022C_LCD_PIN_CS;
        panel_cfg.pin_rst = CYD_2432S022C_LCD_PIN_RST;
        panel_cfg.panel_width = CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION;
        panel_cfg.panel_height = CYD_2432S022C_LCD_VERTICAL_RESOLUTION;
        _panel_instance.config(panel_cfg);

        auto light_cfg = _light_instance.config();
        light_cfg.pin_bl = GPIO_NUM_0;
        _light_instance.config(light_cfg);
        _panel_instance.setLight(&_light_instance);

        setPanel(&_panel_instance);
    }
};

class LovyanGFXDisplay : public tt::hal::display::DisplayDevice {
public:
    struct Configuration {
        uint16_t width, height;
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
        Configuration(uint16_t w, uint16_t h, std::shared_ptr<tt::hal::touch::TouchDevice> t) : width(w), height(h), touch(t) {}
    };

    LovyanGFXDisplay(std::unique_ptr<Configuration> config) : config(std::move(config)) {}
    ~LovyanGFXDisplay() override { stop(); }

   bool start() override {
        ESP_LOGI("LovyanDisplay", "Starting, DRAM free: %u", heap_caps_get_free_size(MALLOC_CAP_DMA));
        lcd.init();
        ESP_LOGI("LovyanDisplay", "LCD init done");
        ESP_LOGI(TAG, "Creating LVGL display: %dx%d", configuration->width, configuration->height);  // 240x320
        lvglDisplay = lv_display_create(config->width, config->height);
        if (!lvglDisplay) {
            ESP_LOGE(TAG, "Failed to create LVGL display");
            return false;
        }
        size_t buffer_size = CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t);
        static uint16_t* buf1 = nullptr;
        static uint16_t* buf2 = nullptr;
        buf1 = (uint16_t*)heap_caps_malloc(buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        buf2 = (uint16_t*)heap_caps_malloc(buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (!buf1 || !buf2) {
            ESP_LOGE(TAG, "Failed to allocate buffers! Size: %d bytes", buffer_size);
            if (buf1) heap_caps_free(buf1);
            if (buf2) heap_caps_free(buf2);
            return false;
        }
        ESP_LOGI(TAG, "Allocated buffers: buf1=%p, buf2=%p, size=%d bytes", buf1, buf2, buffer_size);

        lv_display_set_buffers(lvglDisplay, buf1, buf2, CYD_2432S022C_LCD_DRAW_BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
        // Flush callback using LovyanGFX
        lv_display_set_flush_cb(lvglDisplay, [](lv_display_t* disp, const lv_area_t* area, uint8_t* data) {
            auto* display = static_cast<LovyanGFXDisplay*>(lv_display_get_user_data(disp));
            display->lcd.setWindow(area->x1, area->y1, area->x2, area->y2);
            display->lcd.pushPixels((uint16_t*)data, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1));
            lv_display_flush_ready(disp);
        });

        isStarted = true;
        ESP_LOGI(TAG, "LovyanGFX display started successfully");
        return true;
    }

    bool stop() override { if (lvglDisplay) lv_display_delete(lvglDisplay); lvglDisplay = nullptr; return true; }
    void setBacklightDuty(uint8_t duty) override { lcd.setBrightness(duty); }
    bool supportsBacklightDuty() const override { return true; }
    lv_display_t* getLvglDisplay() const override { return lvglDisplay; }
    std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() override { return config->touch; }
    std::string getName() const override { return "CYD-2432S022C"; }
    std::string getDescription() const override { return "LovyanGFX"; }

private:
    std::unique_ptr<Configuration> config;
    LGFX_CYD_2432S022C lcd;
    lv_display_t* lvglDisplay = nullptr;
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createCST820Touch();
    if (!touch) return nullptr;
    return std::make_shared<LovyanGFXDisplay>(std::make_unique<LovyanGFXDisplay::Configuration>(
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION,
        touch
    ));
}
