#include "devices/Display.h"
#include "devices/SdCard.h"

#include <Tactility/hal/Configuration.h>
#include <Tactility/lvgl/LvglSync.h>
#include <PwmBacklight.h>

static bool initBoot() {
    // Set the RGB LED Pins to output and turn them off
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT)); // Red
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_16, GPIO_MODE_OUTPUT)); // Green
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT)); // Blue

    // 0 on, 1 off
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_4, 1)); // Red
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_16, 1)); // Green
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_17, 1)); // Blue

    return driver::pwmbacklight::init(LCD_PIN_BACKLIGHT);
}

static tt::hal::DeviceVector createDevices() {
    return {
        createDisplay(),
        createSdCard()
    };
}

extern const tt::hal::Configuration hardwareConfiguration = {
    .initBoot = initBoot,
    .createDevices = createDevices,
    .i2c = {
        tt::hal::i2c::Configuration {
            .name = "First",
            .port = I2C_NUM_0,
            .initMode = tt::hal::i2c::InitMode::ByTactility,
            .isMutable = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_33,
                .scl_io_num = GPIO_NUM_32,
                .sda_pullup_en = false,
                .scl_pullup_en = false,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        tt::hal::i2c::Configuration {
            .name = "Second",
            .port = I2C_NUM_1,
            .initMode = tt::hal::i2c::InitMode::Disabled,
            .isMutable = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_NC,
                .scl_io_num = GPIO_NUM_NC,
                .sda_pullup_en = false,
                .scl_pullup_en = false,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        }
    },
    .spi {
        tt::hal::spi::Configuration {
            .device = SPI2_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_13,
                .miso_io_num = GPIO_NUM_NC,
                .sclk_io_num = GPIO_NUM_14,
                .quadwp_io_num = GPIO_NUM_NC, // Quad SPI LCD driver is not yet supported
                .quadhd_io_num = GPIO_NUM_NC, // Quad SPI LCD driver is not yet supported
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = LCD_SPI_TRANSFER_SIZE_LIMIT,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = tt::hal::spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = tt::lvgl::getSyncLock() // esp_lvgl_port owns the lock for the display
        },
        tt::hal::spi::Configuration {
            .device = SPI3_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_23,
                .miso_io_num = GPIO_NUM_19,
                .sclk_io_num = GPIO_NUM_18,
                .quadwp_io_num = GPIO_NUM_NC, // Quad SPI LCD driver is not yet supported
                .quadhd_io_num = GPIO_NUM_NC, // Quad SPI LCD driver is not yet supported
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = 8192,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = tt::hal::spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = nullptr
        },
    }
};
