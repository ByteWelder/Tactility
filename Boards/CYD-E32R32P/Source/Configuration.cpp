#include "devices/SdCard.h"
#include "devices/Display.h"

#include <Tactility/hal/Configuration.h>
#include <Tactility/lvgl/LvglSync.h>
#include <PwmBacklight.h>

#define CYD_SPI_TRANSFER_SIZE_LIMIT (240 * 320 / 4 * 2)

using namespace tt::hal;

static bool initBoot() {
    return driver::pwmbacklight::init(CYD_BACKLIGHT_PIN);
}

static tt::hal::DeviceVector createDevices() {
    return {
        createDisplay(),
        createSdCard()
    };
}

extern const Configuration hardwareConfiguration = {
    .initBoot = initBoot,
    .createDevices = createDevices,
    .i2c = {},
    .spi = {
        tt::hal::spi::Configuration {
            .device = SPI2_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_13, // shared display & touch MOSI
                .miso_io_num = GPIO_NUM_12, // shared display & touch MISO
                .sclk_io_num = GPIO_NUM_14, // shared clock
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
            .initMode = tt::hal::spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = tt::lvgl::getSyncLock()
        },
        tt::hal::spi::Configuration {
            .device = SPI3_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_23, // SD card MOSI
                .miso_io_num = GPIO_NUM_19, // SD card MISO
                .sclk_io_num = GPIO_NUM_18, // SD card SCK
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
            .initMode = tt::hal::spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = tt::lvgl::getSyncLock()
        },

    }
};