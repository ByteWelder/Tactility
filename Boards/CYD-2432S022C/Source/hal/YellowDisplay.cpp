// YellowDisplay.cpp
#include "CYD2432S022CConstants.h"
#include "Tactility/app/display/DisplaySettings.h"
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>

#define TAG "YellowDisplay"

class YellowDisplay : public tt::hal::display::DisplayDevice {
public:
    struct Configuration {
        uint16_t width;
        uint16_t height;
        lv_display_rotation_t rotation;
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
        Configuration(uint16_t width, uint16_t height, lv_display_rotation_t rotation, std::shared_ptr<tt::hal::touch::TouchDevice> touch)
            : width(width), height(height), rotation(rotation), touch(std::move(touch)) {}
    };

    explicit YellowDisplay(std::unique_ptr<Configuration> config) : config(std::move(config)) {
        ESP_LOGI(TAG, "YellowDisplay constructor: %dx%d, rotation=%d", config->width, config->height, config->rotation);
    }

    ~YellowDisplay() override { stop(); }

    bool start() override {
        if (isStarted) {
            ESP_LOGW(TAG, "Display already started");
            return true;
        }

        ESP_LOGI(TAG, "Starting ESP-IDF i80 display for CYD-2432S022C");

        // 1. Configure i80 bus
        esp_lcd_i80_bus_handle_t i80_bus = nullptr;
        esp_lcd_i80_bus_config_t bus_config = {
            .dc_gpio_num = CYD_2432S022C_LCD_PIN_DC,
            .wr_gpio_num = CYD_2432S022C_LCD_PIN_WR,
            .data_gpio_nums = {
                CYD_2432S022C_LCD_PIN_D0, CYD_2432S022C_LCD_PIN_D1, CYD_2432S022C_LCD_PIN_D2, CYD_2432S022C_LCD_PIN_D3,
                CYD_2432S022C_LCD_PIN_D4, CYD_2432S022C_LCD_PIN_D5, CYD_2432S022C_LCD_PIN_D6, CYD_2432S022C_LCD_PIN_D7
            },
            .bus_width = 8,
            .max_transfer_bytes = CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * 2,
            .psram_trans_align = 64,
            .sram_trans_align = 4
        };
        ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

        // 2. Configure ST7789 panel
        esp_lcd_panel_io_i80_config_t io_config = {
            .cs_gpio_num = CYD_2432S022C_LCD_PIN_CS,
            .pclk_hz = 10 * 1000 * 1000,
            .trans_descriptor_num = 10,
            .dc_levels = {.dc_idle_level = 0, .dc_cmd_level = 0, .dc_dummy_level = 0, .dc_data_level = 1},
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &panel_io));

        esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = CYD_2432S022C_LCD_PIN_RST,
            .rgb_endian = LCD_RGB_ENDIAN_RGB,
            .bits_per_pixel = 16
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel_handle));

        // 3. Initialize panel
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, false));
        ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 0));
        setRotation(config->rotation); // Moved before buffer setup

        // 4. Backlight setup (LEDC PWM)
        ledc_timer_config_t ledc_timer = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = LEDC_TIMER_8_BIT,
            .timer_num = LEDC_TIMER_0,
            .freq_hz = 44100,
            .clk_cfg = LEDC_AUTO_CLK
        };
        ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
        ledc_channel_config_t ledc_channel = {
            .gpio_num = CYD_2432S022C_LCD_PIN_BACKLIGHT,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = LEDC_CHANNEL_0,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LEDC_TIMER_0,
            .duty = 0,
            .hpoint = 0
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

        // 5. Buffer setup
        const size_t buffer_width = (config->rotation == LV_DISPLAY_ROTATION_90 || config->rotation == LV_DISPLAY_ROTATION_270)
                                    ? config->height : config->width;
        bufferSize = buffer_width * CYD_2432S022C_LCD_DRAW_BUFFER_HEIGHT;
        buf1 = (uint16_t*)heap_caps_malloc(bufferSize * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        buf2 = (uint16_t*)heap_caps_malloc(bufferSize * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (!buf1 || !buf2) {
            ESP_LOGE(TAG, "Failed to allocate buffers: %d bytes", bufferSize * 2);
            stop();
            return false;
        }

        // 6. LVGL display setup
        lvglDisplay = lv_display_create(config->width, config->height);
        lv_display_set_color_format(lvglDisplay, LV_COLOR_FORMAT_RGB565);
        lv_display_set_buffers(lvglDisplay, buf1, buf2, bufferSize, LV_DISPLAY_RENDER_MODE_PARTIAL);
        lv_display_set_flush_cb(lvglDisplay, [](lv_display_t* disp, const lv_area_t* area, uint8_t* data) {
            auto* display = static_cast<YellowDisplay*>(lv_display_get_user_data(disp));
            if (!display || !data) {
                lv_display_flush_ready(disp);
                return;
            }
            esp_lcd_panel_draw_bitmap(display->panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, data);
            lv_display_flush_ready(disp);
        });

        isStarted = true;
        ESP_LOGI(TAG, "YellowDisplay started");
        return true;
    }

    bool stop() override {
        if (!isStarted) return true;
        if (config && config->touch) config->touch->stop();
        if (lvglDisplay) lv_display_delete(lvglDisplay);
        if (panel_handle) esp_lcd_panel_del(panel_handle);
        if (panel_io) esp_lcd_panel_io_del(panel_io);
        if (buf1) heap_caps_free(buf1);
        if (buf2) heap_caps_free(buf2);
        ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        lvglDisplay = nullptr;
        panel_handle = nullptr;
        panel_io = nullptr;
        buf1 = nullptr;
        buf2 = nullptr;
        isStarted = false;
        return true;
    }

    void setBacklightDuty(uint8_t duty) override {
        if (isStarted) ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    }

    bool supportsBacklightDuty() const override { return true; }

    lv_display_t* getLvglDisplay() const override { return lvglDisplay; }

    std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() override {
        return config ? config->touch : nullptr;
    }

    std::string getName() const override { return "CYD-2432S022C Yellow Display"; }
    std::string getDescription() const override { return "ESP-IDF i80-based display for CYD-2432S022C"; }

private:
    void setRotation(lv_display_rotation_t rotation) {
        bool swapXY = (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270);
        bool mirrorX = (rotation == LV_DISPLAY_ROTATION_270);
        bool mirrorY = (rotation == LV_DISPLAY_ROTATION_90);
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, swapXY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, mirrorX, mirrorY));
        lv_disp_set_rotation(lvglDisplay, rotation);
    }

    std::unique_ptr<Configuration> config;
    esp_lcd_panel_io_handle_t panel_io = nullptr;
    esp_lcd_panel_handle_t panel_handle = nullptr;
    lv_display_t* lvglDisplay = nullptr;
    uint16_t* buf1 = nullptr;
    uint16_t* buf2 = nullptr;
    size_t bufferSize = 0;
    bool isStarted = false;
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createYellowTouch();
    if (!touch) {
        ESP_LOGE(TAG, "Failed to create touch device");
        return nullptr;
    }
    lv_display_rotation_t rotation = tt::app::display::getRotation();
    auto config = std::make_unique<YellowDisplay::Configuration>(
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION,
        rotation,
        touch
    );
    return std::make_shared<YellowDisplay>(std::move(config));
}
