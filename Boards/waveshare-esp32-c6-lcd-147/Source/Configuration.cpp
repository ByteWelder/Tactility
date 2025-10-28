#include "devices/Display.h"
#include "devices/SdCard.h"

#include <PwmBacklight.h>
#include <Tactility/hal/Configuration.h>
#include <Tactility/hal/uart/Uart.h>
#include <Tactility/lvgl/LvglSync.h>

using namespace tt::hal;

static DeviceVector createDevices() {
    return {
        createDisplay(),
        createSdCard()
        // TODO: Add RGB LED device (GPIO8, RMT-based WS2812)
    };
}

extern bool initBoot();

extern const Configuration hardwareConfiguration = {
    .initBoot = initBoot,
    .uiScale = UiScale::Smallest,
    .createDevices = createDevices,
    .i2c = {},
    .spi {
        // Display and SD card (shared SPI bus)
        spi::Configuration {
            .device = LCD_SPI_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = LCD_PIN_MOSI,
                .miso_io_num = LCD_PIN_MISO,
                .sclk_io_num = LCD_PIN_SCLK,
                .quadwp_io_num = GPIO_NUM_NC,
                .quadhd_io_num = GPIO_NUM_NC,
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
            .initMode = spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = tt::lvgl::getSyncLock() // esp_lvgl_port owns the lock for the display
        }
    },
    .uart {}
};
