#include <PwmBacklight.h>
#include "devices/Display.h"

#include <Tactility/hal/Configuration.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace tt::hal;

static bool initBoot() {
    //Display Reset
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_46, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_46, 0));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_46, 1));
    vTaskDelay(pdMS_TO_TICKS(10));

    return driver::pwmbacklight::init(GPIO_NUM_21);
}

static DeviceVector createDevices() {
    return {
        createDisplay()
    };
}

extern const Configuration hardwareConfiguration = {
    .initBoot = initBoot,
    .createDevices = createDevices,
    .i2c = {
        //Touch
        i2c::Configuration {
            .name = "Internal",
            .port = I2C_NUM_0,
            .initMode = i2c::InitMode::ByTactility,
            .isMutable = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_2,
                .scl_io_num = GPIO_NUM_1,
                .sda_pullup_en = true,
                .scl_pullup_en = true,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        //Rear header, JST PH 2.0, SDA IO4 / SCL IO3 / GND / 3.3V
        i2c::Configuration {
            .name = "External",
            .port = I2C_NUM_1,
            .initMode = i2c::InitMode::Disabled,
            .isMutable = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_4,
                .scl_io_num = GPIO_NUM_3,
                .sda_pullup_en = false,
                .scl_pullup_en = false,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        }
    }
};
