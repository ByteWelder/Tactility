#include <RgbDisplay.h>
#include "CYD2432S022CConstants.h"
#include "CST820Touch.h"
#include <esp_log.h>
#include <esp_lcd_panel_rgb.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "RgbDisplay";

// Backlight PWM setup (reusing LovyanGFX’s approach)
static void backlight_init(gpio_num_t pin) {
    esp_rom_gpio_pad_select_gpio(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 44100,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .gpio_num = pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,  // Start off
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static void backlight_set_duty(uint8_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

bool RgbDisplay::start() {
    if (displayHandle) {
        ESP_LOGW(TAG, "Display already started");
        return true;
    }

    ESP_LOGI(TAG, "Starting RGB display");

    // Initialize panel
    ESP_ERROR_CHECK(esp_lcd_panel_rgb_init(&configuration->panelConfig, &panelHandle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panelHandle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panelHandle));

    // Backlight (if configured)
    if (configuration->backlightDutyFunction) {
        configuration->backlightDutyFunction(0);  // Start off
    }

    // LVGL display setup
    ESP_LOGI(TAG, "Creating LVGL display: %dx%d",
             CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION, CYD_2432S022C_LCD_VERTICAL_RESOLUTION);
    displayHandle = lv_display_create(CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION, CYD_2432S022C_LCD_VERTICAL_RESOLUTION);
    if (!displayHandle) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        return false;
    }

    lv_display_set_color_format(displayHandle, configuration->colorFormat);
    lv_display_set_user_data(displayHandle, this);

    // Buffers
    size_t buffer_size = configuration->bufferConfiguration.size;
    if (buffer_size == 0) buffer_size = CYD_2432S022C_LCD_DRAW_BUFFER_SIZE;  // Default
    void* buf1 = nullptr;
    void* buf2 = nullptr;

    if (configuration->bufferConfiguration.useSpi) {
        buf1 = heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
        if (configuration->bufferConfiguration.doubleBuffer) {
            buf2 = heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
        }
    } else {
        buf1 = heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (configuration->bufferConfiguration.doubleBuffer) {
            buf2 = heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        }
    }

    if (!buf1 || (configuration->bufferConfiguration.doubleBuffer && !buf2)) {
        ESP_LOGE(TAG, "Failed to allocate buffers! Size: %d bytes", buffer_size * sizeof(lv_color_t));
        if (buf1) heap_caps_free(buf1);
        if (buf2) heap_caps_free(buf2);
        return false;
    }

    ESP_LOGI(TAG, "Allocated buffers: buf1=%p, buf2=%p, size=%d bytes", buf1, buf2, buffer_size * sizeof(lv_color_t));
    lv_display_set_buffers(displayHandle, buf1, buf2, buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Flush callback
    lv_display_set_flush_cb(displayHandle, [](lv_display_t* disp, const lv_area_t* area, uint8_t* data) {
        auto* display = static_cast<RgbDisplay*>(lv_display_get_user_data(disp));
        esp_lcd_panel_draw_bitmap(display->panelHandle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, data);
        lv_display_flush_ready(disp);
    });

    ESP_LOGI(TAG, "RGB display started successfully");
    return true;
}

bool RgbDisplay::stop() {
    if (!displayHandle) return true;

    if (configuration && configuration->touch) configuration->touch->stop();
    if (displayHandle) lv_display_delete(displayHandle);
    if (panelHandle) esp_lcd_panel_del(panelHandle);
    displayHandle = nullptr;
    panelHandle = nullptr;
    return true;
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createCST820Touch();
    if (!touch) {
        ESP_LOGE(TAG, "Failed to create touch device!");
        return nullptr;
    }

    // ST7789 timing (approximate, tweak if needed)
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = 9000000,  // 9 MHz, adjust for refresh rate
            .h_res = CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION,  // 240
            .v_res = CYD_2432S022C_LCD_VERTICAL_RESOLUTION,    // 320
            .hsync_pulse_width = 10,
            .hsync_back_porch = 20,
            .hsync_front_porch = 50,
            .vsync_pulse_width = 10,
            .vsync_back_porch = 20,
            .vsync_front_porch = 50,
            .flags = {
                .hsync_idle_low = 0,
                .vsync_idle_low = 0,
                .de_idle_high = 0,
                .pclk_active_neg = 0,
                .pclk_idle_high = 0
            }
        },
        .data_width = 8,  // 8-bit parallel
        .bits_per_pixel = 16,  // RGB565
        .num_fbs = 1,
        .bounce_buffer_size_px = 0,
        .sram_trans_align = 0,
        .psram_trans_align = 0,
        .hsync_gpio_num = -1,  // Not used for parallel
        .vsync_gpio_num = -1,
        .de_gpio_num = -1,
        .pclk_gpio_num = CYD_2432S022C_LCD_PIN_WR,  // WR as clock
        .disp_gpio_num = -1,
        .data_gpio_nums = {
            CYD_2432S022C_LCD_PIN_D0, CYD_2432S022C_LCD_PIN_D1, CYD_2432S022C_LCD_PIN_D2, CYD_2432S022C_LCD_PIN_D3,
            CYD_2432S022C_LCD_PIN_D4, CYD_2432S022C_LCD_PIN_D5, CYD_2432S022C_LCD_PIN_D6, CYD_2432S022C_LCD_PIN_D7
        },
        .cs_gpio_num = CYD_2432S022C_LCD_PIN_CS,
        .rd_gpio_num = CYD_2432S022C_LCD_PIN_RD,
        .wr_gpio_num = CYD_2432S022C_LCD_PIN_WR,
        .flags = {
            .disp_active_low = 0,
            .refresh_on_demand = 0,
            .fb_in_psram = 0,
            .double_fb = 0,
            .no_fb = 0,
            .bb_invalidate_cache = 0
        }
    };

    RgbDisplay::BufferConfiguration buffer_config = {
        .size = CYD_2432S022C_LCD_DRAW_BUFFER_SIZE,  // 7680 pixels
        .useSpi = false,  // DMA for speed
        .doubleBuffer = true,
        .bounceBufferMode = false,
        .avoidTearing = false
    };

    backlight_init(GPIO_NUM_0);  // PWM on GPIO 0

    auto config = std::make_unique<RgbDisplay::Configuration>(
        panel_config,
        buffer_config,
        touch,
        LV_COLOR_FORMAT_RGB565,
        true,  // swapXY for landscape
        false, // mirrorX
        false, // mirrorY
        false, // invertColor
        [](uint8_t duty) { backlight_set_duty(duty); }  // Backlight callback
    );

    return std::make_shared<RgbDisplay>(std::move(config));
}
