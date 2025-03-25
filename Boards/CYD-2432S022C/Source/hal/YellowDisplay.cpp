// YellowDisplay.cpp
#include "CYD2432S022CConstants.h"
#include "Tactility/app/display/DisplaySettings.h"
#include "Tactility/hal/display/DisplayDevice.h"
#include "YellowTouch.h"
#include <memory>
#include <string>
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

    explicit YellowDisplay(Configuration* config) {
        ESP_LOGI(TAG, "Constructor entry: this=%p, config=%p", this, config);
        if (!config) {
            ESP_LOGE(TAG, "Constructor: config is null");
            return;
        }
        this->config = std::make_unique<Configuration>(config->width, config->height, config->rotation, config->touch);
        ESP_LOGI(TAG, "YellowDisplay constructor: %dx%d, rotation=%d, touch=%p", 
                 this->config->width, this->config->height, this->config->rotation, this->config->touch.get());
    }

    ~YellowDisplay() override { stop(); }

    bool start() override {
        if (isStarted) {
            ESP_LOGW(TAG, "Display already started");
            return true;
        }
        if (!config) {
            ESP_LOGE(TAG, "start: config is null");
            return false;
        }

        ESP_LOGI(TAG, "Starting ESP-IDF i80 display for CYD-2432S022C");

        // 1. Configure i80 bus
        esp_lcd_i80_bus_handle_t i80_bus = nullptr;
        esp_lcd_i80_bus_config_t bus_config = {
            .dc_gpio_num = CYD_2432S022C_LCD_PIN_DC,
            .wr_gpio_num = CYD_2432S022C_LCD_PIN_WR,
            .clk_src = LCD_CLK_SRC_DEFAULT,
            .data_gpio_nums = {
                CYD_2432S022C_LCD_PIN_D0, CYD_2432S022C_LCD_PIN_D1, CYD_2432S022C_LCD_PIN_D2, CYD_2432S022C_LCD_PIN_D3,
                CYD_2432S022C_LCD_PIN_D4, CYD_2432S022C_LCD_PIN_D5, CYD_2432S022C_LCD_PIN_D6, CYD_2432S022C_LCD_PIN_D7
            },
            .bus_width = 8,
            .max_transfer_bytes = CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * 2,
            .dma_burst_size = 64,
            .sram_trans_align = 4
        };
        esp_err_t err = esp_lcd_new_i80_bus(&bus_config, &i80_bus);
        if (err != ESP_OK || i80_bus == nullptr) {
            ESP_LOGE(TAG, "Failed to create i80 bus: %s", esp_err_to_name(err));
            return false;
        }
        ESP_LOGI(TAG, "i80 bus created: %p", i80_bus);

        // 2. Configure ST7789 panel
        esp_lcd_panel_io_i80_config_t io_config = {
            .cs_gpio_num = CYD_2432S022C_LCD_PIN_CS,
            .pclk_hz = CYD_2432S022C_LCD_PCLK_HZ,
            .trans_queue_depth = CYD_2432S022C_LCD_TRANS_DESCRIPTOR_NUM,
            .on_color_trans_done = nullptr,
            .user_ctx = nullptr,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .dc_levels = {
                .dc_idle_level = 0,
                .dc_cmd_level = 0,
                .dc_dummy_level = 0,
                .dc_data_level = 1
            },
            .flags = {
                .cs_active_high = 0,
                .reverse_color_bits = 0,
                .swap_color_bytes = 0,
                .pclk_active_neg = 0,
                .pclk_idle_low = 0
            }
        };
        err = esp_lcd_new_panel_io_i80(i80_bus, &io_config, &panel_io);
        if (err != ESP_OK || panel_io == nullptr) {
            ESP_LOGE(TAG, "Failed to create panel IO: %s", esp_err_to_name(err));
            esp_lcd_del_i80_bus(i80_bus);
            return false;
        }
        ESP_LOGI(TAG, "Panel IO created: %p", panel_io);

        esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = CYD_2432S022C_LCD_PIN_RST,
            .rgb_endian = LCD_RGB_ENDIAN_RGB,
            .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
            .bits_per_pixel = 16,
            .flags = { .reset_active_high = 0 },
            .vendor_config = nullptr
        };
        err = esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel_handle);
        if (err != ESP_OK || panel_handle == nullptr) {
            ESP_LOGE(TAG, "Failed to create ST7789 panel: %s", esp_err_to_name(err));
            esp_lcd_panel_io_del(panel_io);
            esp_lcd_del_i80_bus(i80_bus);
            return false;
        }
        ESP_LOGI(TAG, "Panel handle created: %p", panel_handle);

        // 3. Initialize panel
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, false));
        ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 0));
        setRotation(config->rotation);

        // 4. Backlight setup (LEDC PWM)
        ledc_timer_config_t ledc_timer = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = CYD_2432S022C_LCD_BACKLIGHT_DUTY_RES,
            .timer_num = CYD_2432S022C_LCD_BACKLIGHT_LEDC_TIMER,
            .freq_hz = CYD_2432S022C_LCD_BACKLIGHT_PWM_FREQ_HZ,
            .clk_cfg = LEDC_AUTO_CLK,
            .deconfigure = false
        };
        ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
        ledc_channel_config_t ledc_channel = {
            .gpio_num = CYD_2432S022C_LCD_PIN_BACKLIGHT,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = CYD_2432S022C_LCD_BACKLIGHT_LEDC_CHANNEL,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = CYD_2432S022C_LCD_BACKLIGHT_LEDC_TIMER,
            .duty = 0,
            .hpoint = 0,
            .sleep_mode = LEDC_ACTIVE_MODE, // Disable sleep mode (keep active)
            .flags = { .output_invert = 0 }
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
        ESP_LOGI(TAG, "Buffers allocated: buf1=%p, buf2=%p, size=%d", buf1, buf2, bufferSize * 2);

        // 6. LVGL display setup
        lvglDisplay = lv_display_create(config->width, config->height);
        if (!lvglDisplay) {
            ESP_LOGE(TAG, "Failed to create LVGL display");
            stop();
            return false;
        }
        ESP_LOGI(TAG, "LVGL display created: %p", lvglDisplay);
        lv_display_set_color_format(lvglDisplay, LV_COLOR_FORMAT_RGB565);
        lv_display_set_buffers(lvglDisplay, buf1, buf2, bufferSize * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
        lv_display_set_user_data(lvglDisplay, this);
        lv_display_set_flush_cb(lvglDisplay, [](lv_display_t* disp, const lv_area_t* area, uint8_t* data) {
            auto* display = static_cast<YellowDisplay*>(lv_display_get_user_data(disp));
            if (!display || !data || !display->panel_handle) {
                ESP_LOGE(TAG, "Flush callback failed: display=%p, data=%p, panel_handle=%p", 
                         display, data, display ? display->panel_handle : nullptr);
                lv_display_flush_ready(disp);
                return;
            }
            esp_lcd_panel_draw_bitmap(display->panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, data);
            lv_display_flush_ready(disp);
        });

        isStarted = true;
        ESP_LOGI(TAG, "YellowDisplay started");
        vTaskDelay(pdMS_TO_TICKS(100));
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
        ledc_stop(LEDC_LOW_SPEED_MODE, CYD_2432S022C_LCD_BACKLIGHT_LEDC_CHANNEL, 0);
        lvglDisplay = nullptr;
        panel_handle = nullptr;
        panel_io = nullptr;
        buf1 = nullptr;
        buf2 = nullptr;
        isStarted = false;
        ESP_LOGI(TAG, "YellowDisplay stopped");
        return true;
    }

    void setBacklightDuty(uint8_t duty) override {
        if (isStarted) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, CYD_2432S022C_LCD_BACKLIGHT_LEDC_CHANNEL, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, CYD_2432S022C_LCD_BACKLIGHT_LEDC_CHANNEL);
        }
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
        lv_display_set_rotation(lvglDisplay, rotation);
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
    ESP_LOGI(TAG, "Creating display");
    auto touch = createYellowTouch();
    if (!touch) {
        ESP_LOGE(TAG, "Failed to create touch device");
        return nullptr;
    }
    ESP_LOGI(TAG, "Touch created: %p", touch.get());
    lv_display_rotation_t rotation = tt::app::display::getRotation();
    ESP_LOGI(TAG, "Rotation retrieved: %d", rotation);

    YellowDisplay::Configuration temp_config(
        CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,
        CYD_2432S022C_LCD_VERTICAL_RESOLUTION,
        rotation,
        touch
    );
    ESP_LOGI(TAG, "Temp config created: width=%d, height=%d, rotation=%d, touch=%p", 
             temp_config.width, temp_config.height, temp_config.rotation, temp_config.touch.get());

    auto display = std::make_shared<YellowDisplay>(&temp_config);
    if (!display) {
        ESP_LOGE(TAG, "Failed to create display object");
        return nullptr;
    }
    ESP_LOGI(TAG, "Display object created: %p", display.get());
    return display;
}
