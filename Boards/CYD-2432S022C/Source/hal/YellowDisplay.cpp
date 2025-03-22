#include <RgbDisplay.h>
#include "CYD2432S022CConstants.h"
#include "CST820Touch.h"
#include <esp_log.h>

// Static TAG for logging
static const char* TAG = "YellowDisplay";

// Backlight PWM setup
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

// Thin wrapper around RgbDisplay
class YellowDisplay : public RgbDisplay {
public:
    explicit YellowDisplay(std::unique_ptr<Configuration> config) : RgbDisplay(std::move(config)) {}
    std::string getName() const override { return "Yellow Display"; }
    std::string getDescription() const override { return "ST7789 8-bit parallel display for CYD-2432S022C"; }
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createCST820Touch();
    if (!touch) {
        ESP_LOGE(TAG, "Failed to create touch device!");
        return nullptr;
    }

    // ST7789 timing and pin config from PlatformIO
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_PLL160M,
        .timings = {
            .pclk_hz = 12000000,
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
        .data_width = 8,
        .bits_per_pixel = 16,
        .num_fbs = 1,
        .bounce_buffer_size_px = 0,
        .sram_trans_align = 4,
        .psram_trans_align = 64,
        .hsync_gpio_num = -1,
        .vsync_gpio_num = -1,
        .de_gpio_num = -1,
        .pclk_gpio_num = CYD_2432S022C_LCD_PIN_WR,  // GPIO_NUM_4
        .disp_gpio_num = -1,
        .data_gpio_nums = {
            CYD_2432S022C_LCD_PIN_D0, CYD_2432S022C_LCD_PIN_D1, CYD_2432S022C_LCD_PIN_D2, CYD_2432S022C_LCD_PIN_D3,
            CYD_2432S022C_LCD_PIN_D4, CYD_2432S022C_LCD_PIN_D5, CYD_2432S022C_LCD_PIN_D6, CYD_2432S022C_LCD_PIN_D7
        },
        .cs_gpio_num = CYD_2432S022C_LCD_PIN_CS,  // GPIO_NUM_17
        .rd_gpio_num = CYD_2432S022C_LCD_PIN_RD,  // GPIO_NUM_2
        .wr_gpio_num = CYD_2432S022C_LCD_PIN_WR,  // GPIO_NUM_4
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
        .size = CYD_2432S022C_LCD_DRAW_BUFFER_SIZE,  // 9600
        .useSpi = false,
        .doubleBuffer = true,
        .bounceBufferMode = false,
        .avoidTearing = false
    };

    // Initialize backlight and provide callback
    backlight_init(CYD_2432S022C_LCD_PIN_BACKLIGHT);  // GPIO_NUM_0

    auto config = std::make_unique<RgbDisplay::Configuration>(
        panel_config,
        buffer_config,
        touch,
        LV_COLOR_FORMAT_RGB565,
        false,  // Portrait by default
        false,
        false,
        false,
        [](uint8_t duty) { backlight_set_duty(duty); }  // Backlight callback
    );

    return std::make_shared<YellowDisplay>(std::move(config));
}
