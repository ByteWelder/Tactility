#include "LovyanDisplay.h"
#include "CYD2432S022CConstants.h"
#include "CST820Touch.h"
#include <LovyanGFX.h>

class LGFX_CYD_2432S022C : public lgfx::LGFX_Device {
public:
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_Parallel8 _bus_instance;
    lgfx::Light_PWM _light_instance;

    LGFX_CYD_2432S022C() {
        _bus_instance.config().pin_wr = CYD_2432S022C_LCD_PIN_WR;
        _bus_instance.config().pin_rd = CYD_2432S022C_LCD_PIN_RD;
        _bus_instance.config().pin_rs = CYD_2432S022C_LCD_PIN_DC;
        _bus_instance.config().pin_d0 = CYD_2432S022C_LCD_PIN_D0;
        _bus_instance.config().pin_d1 = CYD_2432S022C_LCD_PIN_D1;
        _bus_instance.config().pin_d2 = CYD_2432S022C_LCD_PIN_D2;
        _bus_instance.config().pin_d3 = CYD_2432S022C_LCD_PIN_D3;
        _bus_instance.config().pin_d4 = CYD_2432S022C_LCD_PIN_D4;
        _bus_instance.config().pin_d5 = CYD_2432S022C_LCD_PIN_D5;
        _bus_instance.config().pin_d6 = CYD_2432S022C_LCD_PIN_D6;
        _bus_instance.config().pin_d7 = CYD_2432S022C_LCD_PIN_D7;
        _panel_instance.setBus(&_bus_instance);

        _panel_instance.config().pin_cs = CYD_2432S022C_LCD_PIN_CS;
        _panel_instance.config().pin_rst = CYD_2432S022C_LCD_PIN_RST;
        _panel_instance.config().panel_width = CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION;
        _panel_instance.config().panel_height = CYD_2432S022C_LCD_VERTICAL_RESOLUTION;

        _light_instance.config().pin_bl = GPIO_NUM_0;
        _panel_instance.setLight(&_light_instance);

        setPanel(&_panel_instance);
    }
};

class LovyanGFXDisplay : public tt::hal::display::DisplayDevice {
public:
    struct Configuration {
        uint16_t width;
        uint16_t height;
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
        Configuration(uint16_t width, uint16_t height, std::shared_ptr<tt::hal::touch::TouchDevice> touch)
            : width(width), height(height), touch(touch) {}
    };

    explicit LovyanGFXDisplay(std::unique_ptr<Configuration> config) : configuration(std::move(config)) {}
    ~LovyanGFXDisplay() override { stop(); }

    bool start() override {
        if (isStarted) return true;
        lcd.init();
        lcd.setBrightness(0);

        lvglDisplay = lv_display_create(configuration->width, configuration->height);
        lv_display_set_color_format(lvglDisplay, LV_COLOR_FORMAT_RGB565);

        static uint16_t* buf1 = (uint16_t*)heap_caps_malloc(CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        static uint16_t* buf2 = (uint16_t*)heap_caps_malloc(CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (!buf1 || !buf2) return false;

        lv_display_set_buffers(lvglDisplay, buf1, buf2, CYD_2432S022C_LCD_DRAW_BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

        lv_display_set_flush_cb(lvglDisplay, [](lv_display_t* disp, const lv_area_t* area, uint8_t* data) {
            auto* display = static_cast<LovyanGFXDisplay*>(lv_display_get_user_data(disp));
            display->lcd.setWindow(area->x1, area->y1, area->x2, area->y2);
            display->lcd.pushPixels((uint16_t*)data, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1));
            lv_display_flush_ready(disp);
        });

        isStarted = true;
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

    void setBacklightDuty(uint8_t backlightDuty) override { if (isStarted) lcd.setBrightness(backlightDuty); }
    bool supportsBacklightDuty() const override { return true; }
    lv_display_t* getLvglDisplay() const override { return lvglDisplay; }
    std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() override { return configuration ? configuration->touch : nullptr; }
    std::string getName() const override { return "CYD-2432S022C Display"; }
    std::string getDescription() const override { return "LovyanGFX-based display"; }

private:
    std::unique_ptr<Configuration> configuration;
    LGFX_CYD_2432S022C lcd;
    lv_display_t* lvglDisplay = nullptr;
    bool isStarted = false;
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createCST820Touch();
    if (!touch) return nullptr;
    auto config = std::make_unique<LovyanGFXDisplay::Configuration>(
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION,
        touch
    );
    return std::make_shared<LovyanGFXDisplay>(std::move(config));
}
