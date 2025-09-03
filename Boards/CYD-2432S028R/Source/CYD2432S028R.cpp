#include "CYD2432S028R.h"
#include "devices/Display.h"
#include "devices/SdCard.h"
#include <Tactility/lvgl/LvglSync.h>
#include <PwmBacklight.h>
#include <Tactility/hal/Configuration.h>

// SPI Transfer
#define CYD_SPI_TRANSFER_SIZE_LIMIT (CYD2432S028R_LCD_DRAW_BUFFER_SIZE * LV_COLOR_DEPTH / 8)
// Display backlight (PWM)
#define CYD2432S028R_LCD_PIN_BACKLIGHT GPIO_NUM_21

using namespace tt::hal;

static bool initBoot() {
    //Set the RGB Led Pins to output and turn them off
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT)); //Red
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_16, GPIO_MODE_OUTPUT)); //Green
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT)); //Blue

    //0 on, 1 off... yep it's backwards.
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_4, 1)); //Red
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_16, 1)); //Green
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_17, 1)); //Blue

    return driver::pwmbacklight::init(CYD2432S028R_LCD_PIN_BACKLIGHT);
}

static DeviceVector createDevices() {
    return {
        createDisplay(),
        createSdCard()
    };
}

const Configuration cyd_2432s028r_config = {
    .initBoot = initBoot,
    .createDevices = createDevices,
    .i2c = {},
    .spi {
        //Display
        spi::Configuration {
            .device = SPI2_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_13,
                .miso_io_num = GPIO_NUM_12,
                .sclk_io_num = GPIO_NUM_14,
                .quadwp_io_num = GPIO_NUM_NC,
                .quadhd_io_num = GPIO_NUM_NC,
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = CYD_SPI_TRANSFER_SIZE_LIMIT,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = tt::lvgl::getSyncLock()
        },
        
        // SDCard
        spi::Configuration {
            .device = SPI3_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_23,
                .miso_io_num = GPIO_NUM_19,
                .sclk_io_num = GPIO_NUM_18,
                .quadwp_io_num = GPIO_NUM_NC,
                .quadhd_io_num = GPIO_NUM_NC,
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = 0,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = tt::lvgl::getSyncLock() // esp_lvgl_port owns the lock for the display
        },
    }
};
