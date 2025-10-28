#include "devices/Display.h"
#include "devices/Power.h"
#include "devices/Constants.h"

#include <Tactility/hal/Configuration.h>
#include <Tactility/lvgl/LvglSync.h>
#include <ButtonControl.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void enableOledPower() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << HELTEC_LCD_PIN_POWER);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE; // The board has an external pull-up
    gpio_config(&io_conf);
    gpio_set_level(HELTEC_LCD_PIN_POWER, 0); // Active low

    vTaskDelay(pdMS_TO_TICKS(500)); // Add a small delay for power to stabilize
    ESP_LOGI("OLED_POWER", "OLED power enabled");
}

static bool initBoot() {
    // Enable power to the OLED before doing anything else
    enableOledPower();

    return true;
}

using namespace tt::hal;

static std::vector<std::shared_ptr<Device>> createDevices() {
    return {
        createPower(),
        ButtonControl::createOneButtonControl(0),
        createDisplay()
    };
}

extern const Configuration hardwareConfiguration = {
    .initBoot = initBoot,
    .uiScale = UiScale::Smallest,
    .createDevices = createDevices,
    .i2c = {
        tt::hal::i2c::Configuration {
            .name = "SSD1306_I2C",
            .port = HELTEC_LCD_I2C_PORT,
            .initMode = tt::hal::i2c::InitMode::ByTactility,
            .isMutable = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = HELTEC_LCD_PIN_SDA,
                .scl_io_num = HELTEC_LCD_PIN_SCL,
                .sda_pullup_en = GPIO_PULLUP_ENABLE,
                .scl_pullup_en = GPIO_PULLUP_ENABLE,
                .master = {
                    .clk_speed = HELTEC_LCD_I2C_SPEED
                },
                .clk_flags = 0
            }
        }
    },
    .spi {},
};
