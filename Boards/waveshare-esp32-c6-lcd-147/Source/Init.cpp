#include <driver/ledc.h>
#include <esp_err.h>
#include <driver/gpio.h>

// Backlight configuration based on demo code
#define LCD_BL_PIN GPIO_NUM_22
#define LCD_BL_LEDC_MODE LEDC_LOW_SPEED_MODE
#define LCD_BL_LEDC_TIMER LEDC_TIMER_0
#define LCD_BL_LEDC_CHANNEL LEDC_CHANNEL_0
#define LCD_BL_LEDC_RESOLUTION LEDC_TIMER_13_BIT  // 13-bit resolution (8192 levels)
#define LCD_BL_LEDC_DUTY ((1 << LCD_BL_LEDC_RESOLUTION) - 1)  // Max duty = 8191
#define LCD_BL_FREQUENCY 5000  // 5 kHz

namespace driver::pwmbacklight {

    bool init(gpio_num_t pin, uint32_t maxDuty) {
        // Configure LEDC timer
        ledc_timer_config_t timer_config = {
            .speed_mode = LCD_BL_LEDC_MODE,
            .duty_resolution = LCD_BL_LEDC_RESOLUTION,
            .timer_num = LCD_BL_LEDC_TIMER,
            .freq_hz = LCD_BL_FREQUENCY,
            .clk_cfg = LEDC_AUTO_CLK
        };
        esp_err_t err = ledc_timer_config(&timer_config);
        if (err != ESP_OK) {
            return false;
        }

        // Configure LEDC channel
        ledc_channel_config_t channel_config = {
            .gpio_num = LCD_BL_PIN,
            .speed_mode = LCD_BL_LEDC_MODE,
            .channel = LCD_BL_LEDC_CHANNEL,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LCD_BL_LEDC_TIMER,
            .duty = 0,  // Start with backlight off
            .hpoint = 0,
            .flags = {
                .output_invert = 0
            }
        };
        err = ledc_channel_config(&channel_config);
        if (err != ESP_OK) {
            return false;
        }

        return true;
    }

    void setBacklightDuty(uint8_t level) {
        // Convert 0-255 level to duty cycle with inverted mapping as in demo code
        // The demo uses: duty = max - (81 * (100 - percentage))
        // We'll adapt this for 0-255 range
        if (level > 255) level = 255;

        // Map 0-255 to 0-100 percentage
        uint8_t percentage = (level * 100) / 255;

        // Calculate duty with inverted logic (higher level = higher duty)
        uint32_t duty;
        if (percentage == 0) {
            duty = 0;
        } else {
            // Map to full range: 0% -> 0, 100% -> LCD_BL_LEDC_DUTY
            duty = LCD_BL_LEDC_DUTY - ((81 * (100 - percentage)));
        }

        // Ensure duty doesn't exceed max
        if (duty > LCD_BL_LEDC_DUTY) {
            duty = LCD_BL_LEDC_DUTY;
        }

        ledc_set_duty(LCD_BL_LEDC_MODE, LCD_BL_LEDC_CHANNEL, duty);
        ledc_update_duty(LCD_BL_LEDC_MODE, LCD_BL_LEDC_CHANNEL);
    }

} // namespace driver::pwmbacklight

bool initBoot() {
    return driver::pwmbacklight::init(LCD_BL_PIN, 256);
}
