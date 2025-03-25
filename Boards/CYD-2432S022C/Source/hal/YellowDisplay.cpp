#include "CYD2432S022CConstants.h"
#include "Tactility/hal/display/DisplayDevice.h"
#include "YellowTouch.h"
#include "Tactility/app/display/DisplaySettings.h"
#include <memory>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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
        ESP_LOGI(TAG, "Constructor: this=%p, config=%p", this, config);
        if (!config) {
            ESP_LOGE(TAG, "Constructor: config is null");
            return;
        }
        this->config = std::make_unique<Configuration>(config->width, config->height, config->rotation, config->touch);
        ESP_LOGI(TAG, "YellowDisplay: %dx%d, rotation=%d, touch=%p", 
                 config->width, config->height, config->rotation, config->touch.get());
    }

    ~YellowDisplay() override { stop(); }

    bool start() override {
        ESP_LOGI(TAG, "start: Entering");
        if (isStarted) {
            ESP_LOGW(TAG, "start: Display already started");
            return true;
        }
        if (!config) {
            ESP_LOGE(TAG, "start: config is null");
            return false;
        }

        ESP_LOGI(TAG, "start: Starting ESP-IDF i80 display");

        // RD pin setup
        ESP_LOGI(TAG, "start: Setting RD pin %d", CYD_2432S022C_LCD_PIN_RD);
        ESP_ERROR_CHECK(gpio_set_direction(CYD_2432S022C_LCD_PIN_RD, GPIO_MODE_OUTPUT));
        ESP_ERROR_CHECK(gpio_set_level(CYD_2432S022C_LCD_PIN_RD, 1));
        ESP_LOGI(TAG, "start: RD pin %d set high", CYD_2432S022C_LCD_PIN_RD);

        // i80 bus setup
        ESP_LOGI(TAG, "start: Setting up i80 bus");
        esp_lcd_i80_bus_handle_t i80_bus = nullptr;
        esp_lcd_i80_bus_config_t bus_config = {
            .dc_gpio_num = CYD_2432S022C_LCD_PIN_DC,
            .wr_gpio_num = CYD_2432S022C_LCD_PIN_WR,
            .clk_src = LCD_CLK_SRC_DEFAULT,
            .data_gpio_nums = {CYD_2432S022C_LCD_PIN_D0, CYD_2432S022C_LCD_PIN_D1, CYD_2432S022C_LCD_PIN_D2, CYD_2432S022C_LCD_PIN_D3,
                               CYD_2432S022C_LCD_PIN_D4, CYD_2432S022C_LCD_PIN_D5, CYD_2432S022C_LCD_PIN_D6, CYD_2432S022C_LCD_PIN_D7},
            .bus_width = 8,
            .max_transfer_bytes = CYD_2432S022C_LCD_DRAW_BUFFER_SIZE * 2,
            .dma_burst_size = 64,
            .sram_trans_align = 4
        };
        ESP_LOGI(TAG, "start: Bus config - DC=%d, WR=%d, D0-D7=[%d,%d,%d,%d,%d,%d,%d,%d], max_bytes=%d",
                 bus_config.dc_gpio_num, bus_config.wr_gpio_num,
                 bus_config.data_gpio_nums[0], bus_config.data_gpio_nums[1], bus_config.data_gpio_nums[2],
                 bus_config.data_gpio_nums[3], bus_config.data_gpio_nums[4], bus_config.data_gpio_nums[5],
                 bus_config.data_gpio_nums[6], bus_config.data_gpio_nums[7], bus_config.max_transfer_bytes);
        ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));
        ESP_LOGI(TAG, "start: i80 bus created: %p", i80_bus);

        // Panel IO setup
        ESP_LOGI(TAG, "start: Setting up panel IO");
        esp_lcd_panel_io_i80_config_t io_config = {
            .cs_gpio_num = CYD_2432S022C_LCD_PIN_CS,
            .pclk_hz = CYD_2432S022C_LCD_PCLK_HZ,
            .trans_queue_depth = CYD_2432S022C_LCD_TRANS_DESCRIPTOR_NUM,
            .on_color_trans_done = flush_ready_callback,
            .user_ctx = this,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .dc_levels = {.dc_idle_level = 0, .dc_cmd_level = 0, .dc_dummy_level = 0, .dc_data_level = 1},
            .flags = {.cs_active_high = 0, .reverse_color_bits = 0, .swap_color_bytes = 0, .pclk_active_neg = 0, .pclk_idle_low = 0}
        };
        ESP_LOGI(TAG, "start: IO config - CS=%d, PCLK=%d, queue_depth=%d",
                 io_config.cs_gpio_num, io_config.pclk_hz, io_config.trans_queue_depth);
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &panel_io));
        ESP_LOGI(TAG, "start: Panel IO created: %p", panel_io);

        // ST7789 panel setup
        ESP_LOGI(TAG, "start: Setting up ST7789 panel");
        esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = CYD_2432S022C_LCD_PIN_RST,
            .rgb_endian = LCD_RGB_ENDIAN_RGB,
            .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
            .bits_per_pixel = 16,
            .flags = {.reset_active_high = 0},
            .vendor_config = nullptr
        };
        ESP_LOGI(TAG, "start: Panel config - RST=%d, RGB=%d, bits=%d",
                 panel_config.reset_gpio_num, panel_config.rgb_endian, panel_config.bits_per_pixel);
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel_handle));
        ESP_LOGI(TAG, "start: Panel handle created: %p", panel_handle);

        ESP_LOGI(TAG, "start: Resetting panel");
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
        ESP_LOGI(TAG, "start: Initializing panel");
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
        ESP_LOGI(TAG, "start: Setting invert color false");
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, false));
        ESP_LOGI(TAG, "start: Setting gap 0,0");
        ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 0));
        ESP_LOGI(TAG, "start: ST7789 init done");

        ESP_LOGI(TAG, "start: Setting rotation");
        setRotation(config->rotation);

        // Backlight setup
        ESP_LOGI(TAG, "start: Setting up backlight");
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
            .duty = 200,
            .hpoint = 0,
            .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
            .flags = {.output_invert = 0}
        };
        ESP_LOGI(TAG, "start: Backlight config - GPIO=%d, duty=%d",
                 ledc_channel.gpio_num, ledc_channel.duty);
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
        ESP_LOGI(TAG, "start: Backlight duty set: 200");

        // Buffer setup
        ESP_LOGI(TAG, "start: Setting up buffers");
        const size_t buffer_width = (config->rotation == LV_DISPLAY_ROTATION_90 || config->rotation == LV_DISPLAY_ROTATION_270)
                                    ? config->height : config->width;
        bufferSize = buffer_width * CYD_2432S022C_LCD_DRAW_BUFFER_HEIGHT;
        buf1 = (uint16_t*)heap_caps_malloc(bufferSize * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        buf2 = (uint16_t*)heap_caps_malloc(bufferSize * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (!buf1 || !buf2) {
            ESP_LOGE(TAG, "start: Failed to allocate buffers: %d bytes", bufferSize * 2);
            stop();
            return false;
        }
        ESP_LOGI(TAG, "start: Buffers allocated: buf1=%p, buf2=%p, size=%d", buf1, buf2, bufferSize * 2);

        // LVGL setup
        ESP_LOGI(TAG, "start: Setting up LVGL");
        lvglDisplay = lv_display_create(config->width, config->height);
        if (!lvglDisplay) {
            ESP_LOGE(TAG, "start: Failed to create LVGL display");
            stop();
            return false;
        }
        ESP_LOGI(TAG, "start: LVGL display created: %p", lvglDisplay);
        ESP_LOGI(TAG, "start: Setting color format RGB565");
        lv_display_set_color_format(lvglDisplay, LV_COLOR_FORMAT_RGB565);
        ESP_LOGI(TAG, "start: Setting buffers");
        lv_display_set_buffers(lvglDisplay, buf1, buf2, bufferSize * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
        ESP_LOGI(TAG, "start: Setting flush callback");
        lv_display_set_flush_cb(lvglDisplay, flush_callback);
        s_display = this;

        isStarted = true;
        ESP_LOGI(TAG, "start: YellowDisplay started");

        // Test flush: 240x16 blue strip
        ESP_LOGI(TAG, "start: Preparing test flush");
        for (size_t i = 0; i < 240 * 16; i++) {
            buf1[i] = 0x001F;  // RGB565 Blue
        }
        ESP_LOGI(TAG, "start: Data sample pre-swap: %04x", buf1[0]);
        ESP_LOGI(TAG, "start: Sending test flush (240x16 blue)");
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 240, 16, buf1));
        ESP_LOGI(TAG, "start: Test flush sent");
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "start: Exiting");
        return true;
    }

    bool stop() override {
        ESP_LOGI(TAG, "stop: Entering");
        if (!isStarted) {
            ESP_LOGI(TAG, "stop: Not started, exiting");
            return true;
        }
        if (config && config->touch) {
            ESP_LOGI(TAG, "stop: Stopping touch");
            config->touch->stop();
        }
        if (lvglDisplay) {
            ESP_LOGI(TAG, "stop: Deleting LVGL display");
            lv_display_delete(lvglDisplay);
        }
        if (panel_handle) {
            ESP_LOGI(TAG, "stop: Deleting panel");
            ESP_ERROR_CHECK(esp_lcd_panel_del(panel_handle));
        }
        if (panel_io) {
            ESP_LOGI(TAG, "stop: Deleting panel IO");
            ESP_ERROR_CHECK(esp_lcd_panel_io_del(panel_io));
        }
        if (buf1) {
            ESP_LOGI(TAG, "stop: Freeing buf1");
            heap_caps_free(buf1);
        }
        if (buf2) {
            ESP_LOGI(TAG, "stop: Freeing buf2");
            heap_caps_free(buf2);
        }
        ESP_LOGI(TAG, "stop: Stopping backlight");
        ESP_ERROR_CHECK(ledc_stop(LEDC_LOW_SPEED_MODE, CYD_2432S022C_LCD_BACKLIGHT_LEDC_CHANNEL, 0));
        lvglDisplay = nullptr;
        panel_handle = nullptr;
        panel_io = nullptr;
        buf1 = nullptr;
        buf2 = nullptr;
        s_display = nullptr;
        isStarted = false;
        ESP_LOGI(TAG, "stop: YellowDisplay stopped");
        ESP_LOGI(TAG, "stop: Exiting");
        return true;
    }

    void setBacklightDuty(uint8_t duty) override {
        ESP_LOGI(TAG, "setBacklightDuty: Entering, duty=%d", duty);
        if (isStarted) {
            ESP_LOGI(TAG, "setBacklightDuty: Setting duty to %d", duty);
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, CYD_2432S022C_LCD_BACKLIGHT_LEDC_CHANNEL, duty));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, CYD_2432S022C_LCD_BACKLIGHT_LEDC_CHANNEL));
        } else {
            ESP_LOGW(TAG, "setBacklightDuty: Display not started");
        }
        ESP_LOGI(TAG, "setBacklightDuty: Exiting");
    }

    bool supportsBacklightDuty() const override {
        ESP_LOGI(TAG, "supportsBacklightDuty: Returning true");
        return true;
    }
    lv_display_t* getLvglDisplay() const override {
        ESP_LOGI(TAG, "getLvglDisplay: Returning %p", lvglDisplay);
        return lvglDisplay;
    }
    std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() override {
        ESP_LOGI(TAG, "createTouch: Returning %p", config ? config->touch.get() : nullptr);
        return config ? config->touch : nullptr;
    }
    std::string getName() const override {
        ESP_LOGI(TAG, "getName: Returning CYD-2432S022C Yellow Display");
        return "CYD-2432S022C Yellow Display";
    }
    std::string getDescription() const override {
        ESP_LOGI(TAG, "getDescription: Returning ESP-IDF i80-based display");
        return "ESP-IDF i80-based display";
    }

private:
    static YellowDisplay* s_display;

    static void flush_callback(lv_display_t* disp, const lv_area_t* area, uint8_t* data) {
        ESP_LOGI(TAG, "flush_callback: Entering, disp=%p, area=[%ld,%ld,%ld,%ld], data=%p",
                 disp, (long)area->x1, (long)area->y1, (long)area->x2, (long)area->y2, data);
        if (!s_display || !s_display->panel_handle) {
            ESP_LOGE(TAG, "flush_callback: Display or panel_handle is null");
            lv_display_flush_ready(disp);
            ESP_LOGI(TAG, "flush_callback: Exiting (null check)");
            return;
        }
        if (!data) {
            ESP_LOGE(TAG, "flush_callback: Data is null");
            lv_display_flush_ready(disp);
            ESP_LOGI(TAG, "flush_callback: Exiting (data null)");
            return;
        }
        uint16_t* p = (uint16_t*)data;
        uint32_t pixels = lv_area_get_size(area);
        ESP_LOGI(TAG, "flush_callback: Pre-swap sample: %04x, pixels=%lu", p[0], (unsigned long)pixels);
        for (uint32_t i = 0; i < pixels; i++) {
            p[i] = (p[i] >> 8) | (p[i] << 8);
        }
        ESP_LOGI(TAG, "flush_callback: Post-swap sample: %04x", p[0]);
        ESP_LOGI(TAG, "flush_callback: Drawing bitmap");
        esp_err_t err = esp_lcd_panel_draw_bitmap(s_display->panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, data);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "flush_callback: Draw bitmap failed: %d", err);
        }
        ESP_LOGI(TAG, "flush_callback: Flush ready");
        lv_display_flush_ready(disp);
        ESP_LOGI(TAG, "flush_callback: Exiting");
    }

    static bool flush_ready_callback(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t* edata, void* user_ctx) {
        ESP_LOGI(TAG, "flush_ready_callback: DMA transfer done");
        return false;
    }

    void setRotation(lv_display_rotation_t rotation) {
        ESP_LOGI(TAG, "setRotation: Entering, rotation=%d", rotation);
        bool swapXY = (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270);
        bool mirrorX = (rotation == LV_DISPLAY_ROTATION_270);
        bool mirrorY = (rotation == LV_DISPLAY_ROTATION_90);
        ESP_LOGI(TAG, "setRotation: swapXY=%d, mirrorX=%d, mirrorY=%d", swapXY, mirrorX, mirrorY);
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, swapXY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, mirrorX, mirrorY));
        lv_display_set_rotation(lvglDisplay, rotation);
        ESP_LOGI(TAG, "setRotation: Exiting");
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

YellowDisplay* YellowDisplay::s_display = nullptr;

static std::shared_ptr<YellowDisplay> globalDisplay;

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    ESP_LOGI(TAG, "createDisplay: Entering");
    ESP_LOGI(TAG, "createDisplay: Creating touch");
    auto touch = createYellowTouch();
    if (!touch) {
        ESP_LOGE(TAG, "createDisplay: Failed to create touch");
        return nullptr;
    }
    lv_display_rotation_t rotation = tt::app::display::getRotation();
    ESP_LOGI(TAG, "createDisplay: Rotation=%d", rotation);
    YellowDisplay::Configuration config(CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION, CYD_2432S022C_LCD_VERTICAL_RESOLUTION, rotation, touch);
    ESP_LOGI(TAG, "createDisplay: Creating display object");
    globalDisplay = std::make_shared<YellowDisplay>(&config);
    if (!globalDisplay) {
        ESP_LOGE(TAG, "createDisplay: Failed to create display");
        return nullptr;
    }
    ESP_LOGI(TAG, "createDisplay: Display created: %p", globalDisplay.get());
    ESP_LOGI(TAG, "createDisplay: Exiting");
    return globalDisplay;
}
