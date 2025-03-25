#include "CYD2432S022CConstants.h"
#include "Tactility/hal/display/DisplayDevice.h"
#include "YellowTouch.h"
#include <memory>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
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
        if (isStarted) {
            ESP_LOGW(TAG, "Display already started");
            return true;
        }
        if (!config) {
            ESP_LOGE(TAG, "start: config is null");
            return false;
        }

        ESP_LOGI(TAG, "Starting ESP-IDF i80 display");

        // i80 bus setup
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
        ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));
        ESP_LOGI(TAG, "i80 bus: %p", i80_bus);

        // Panel IO setup
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
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &panel_io));
        ESP_LOGI(TAG, "Panel IO: %p", panel_io);

        // ST7789 panel setup
        esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = CYD_2432S022C_LCD_PIN_RST,
            .rgb_endian = LCD_RGB_ENDIAN_RGB,
            .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
            .bits_per_pixel = 16,
            .flags = {.reset_active_high = 0},
            .vendor_config = nullptr
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel_handle));
        ESP_LOGI(TAG, "Panel handle: %p", panel_handle);

        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, false));
        ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 0));
        setRotation(config->rotation);

        // Backlight setup (GPIO 0)
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
            .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
            .flags = {.output_invert = 0}
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

        // Buffer setup
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
        ESP_LOGI(TAG, "Buffers: buf1=%p, buf2=%p, size=%d", buf1, buf2, bufferSize * 2);

        // LVGL driver setup
        lv_disp_draw_buf_init(&draw_buf, buf1, buf2, bufferSize);
        lv_disp_drv_init(&disp_drv);
        disp_drv.hor_res = config->width;
        disp_drv.ver_res = config->height;
        disp_drv.flush_cb = flush_callback;
        disp_drv.draw_buf = &draw_buf;
        disp_drv.user_data = this;
        lvglDisplay = lv_disp_drv_register(&disp_drv);
        if (!lvglDisplay) {
            ESP_LOGE(TAG, "Failed to register LVGL display");
            stop();
            return false;
        }
        ESP_LOGI(TAG, "LVGL display: %p, user_data=%p", lvglDisplay, lv_disp_get_user_data(lvglDisplay));

        flush_sem = xSemaphoreCreateBinary();
        if (!flush_sem) {
            ESP_LOGE(TAG, "Failed to create flush semaphore");
            stop();
            return false;
        }
        xSemaphoreGive(flush_sem);  // Initially free

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
        if (flush_sem) vSemaphoreDelete(flush_sem);
        ledc_stop(LEDC_LOW_SPEED_MODE, CYD_2432S022C_LCD_BACKLIGHT_LEDC_CHANNEL, 0);
        lvglDisplay = nullptr;
        panel_handle = nullptr;
        panel_io = nullptr;
        buf1 = nullptr;
        buf2 = nullptr;
        flush_sem = nullptr;
        isStarted = false;
        ESP_LOGI(TAG, "YellowDisplay stopped");
        return true;
    }

    void setBacklightDuty(uint8_t duty) override {
        if (isStarted) {
            ESP_LOGI(TAG, "Backlight duty: %d", duty);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, CYD_2432S022C_LCD_BACKLIGHT_LEDC_CHANNEL, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, CYD_2432S022C_LCD_BACKLIGHT_LEDC_CHANNEL);
        }
    }

    bool supportsBacklightDuty() const override { return true; }
    lv_display_t* getLvglDisplay() const override { return lvglDisplay; }
    std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() override { return config ? config->touch : nullptr; }
    std::string getName() const override { return "CYD-2432S022C Yellow Display"; }
    std::string getDescription() const override { return "ESP-IDF i80-based display"; }

private:
    static void flush_callback(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_map) {
        auto* display = static_cast<YellowDisplay*>(drv->user_data);
        if (!display) {
            ESP_LOGE(TAG, "Flush: display is null");
            lv_disp_flush_ready(drv);
            return;
        }
        ESP_LOGD(TAG, "Flush: [%ld,%ld,%ld,%ld]", 
                 (long)area->x1, (long)area->y1, (long)area->x2, (long)area->y2);
        esp_lcd_panel_draw_bitmap(display->panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
        // Wait for flush completion in callback
    }

    static bool flush_ready_callback(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t* edata, void* user_ctx) {
        auto* display = static_cast<YellowDisplay*>(user_ctx);
        if (display && display->lvglDisplay && display->flush_sem) {
            xSemaphoreGiveFromISR(display->flush_sem, nullptr);
            lv_display_flush_ready(display->lvglDisplay);
        }
        return false;
    }

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
    lv_disp_draw_buf_t draw_buf;
    lv_disp_drv_t disp_drv;
    uint16_t* buf1 = nullptr;
    uint16_t* buf2 = nullptr;
    size_t bufferSize = 0;
    SemaphoreHandle_t flush_sem = nullptr;
    bool isStarted = false;
};

static std::shared_ptr<YellowDisplay> globalDisplay;

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    ESP_LOGI(TAG, "Creating display");
    auto touch = createYellowTouch();
    if (!touch) {
        ESP_LOGE(TAG, "Failed to create touch");
        return nullptr;
    }
    lv_display_rotation_t rotation = tt::app::display::getRotation();
    YellowDisplay::Configuration config(CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION, CYD_2432S022C_LCD_VERTICAL_RESOLUTION, rotation, touch);
    globalDisplay = std::make_shared<YellowDisplay>(&config);
    if (!globalDisplay) {
        ESP_LOGE(TAG, "Failed to create display");
        return nullptr;
    }
    ESP_LOGI(TAG, "Display created: %p", globalDisplay.get());
    return globalDisplay;
}
