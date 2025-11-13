#include "devices/Display.h"
#include "devices/Power.h"
#include "devices/Constants.h"

#include <Tactility/hal/Configuration.h>
#include <Tactility/lvgl/LvglSync.h>
#include <ButtonControl.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include <Tactility/Log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void enableOledPower() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DISPLAY_PIN_POWER),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE, // The board has an external pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(DISPLAY_PIN_POWER, 0); // Active low

    vTaskDelay(pdMS_TO_TICKS(500)); // Add a small delay for power to stabilize
    TT_LOG_I("OLED_POWER", "OLED power enabled");
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
            .name = "Internal",
            .port = DISPLAY_I2C_PORT,
            .initMode = tt::hal::i2c::InitMode::ByTactility,
            .isMutable = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = DISPLAY_PIN_SDA,
                .scl_io_num = DISPLAY_PIN_SCL,
                .sda_pullup_en = GPIO_PULLUP_ENABLE,
                .scl_pullup_en = GPIO_PULLUP_ENABLE,
                .master = {
                    .clk_speed = DISPLAY_I2C_SPEED
                },
                .clk_flags = 0
            }
        }
    },
    .spi {},
};
