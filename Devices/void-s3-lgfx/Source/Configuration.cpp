#include "devices/Display.h"
#include <Tactility/hal/Configuration.h>
#include <Tactility/lvgl/LvglSync.h>

using namespace tt::hal;

static tt::hal::DeviceVector createDevices() {
    tt::hal::DeviceVector devices;
    devices.push_back(tt::hal::display::createDisplay());
    return devices;
}

extern const Configuration hardwareConfiguration = {
    .createDevices = createDevices,
    // spi is initialized by lovyangfx
    // .spi = {
    //     tt::hal::spi::Configuration {
    //         .device = SPI2_HOST,
    //         .dma = SPI_DMA_CH_AUTO,
    //         .config = {
    //             .mosi_io_num = GPIO_NUM_11,
    //             .miso_io_num = GPIO_NUM_7,
    //             .sclk_io_num = GPIO_NUM_12,
    //             .quadwp_io_num = -1, // Quad SPI LCD driver not used
    //             .quadhd_io_num = -1, // Quad SPI LCD driver not used
    //             .data4_io_num = GPIO_NUM_NC,
    //             .data5_io_num = GPIO_NUM_NC,
    //             .data6_io_num = GPIO_NUM_NC,
    //             .data7_io_num = GPIO_NUM_NC,
    //             .data_io_default_level = false,
    //             .max_transfer_sz = 240 * 320 * sizeof(lv_color_t),
    //             .flags = 0,
    //             .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
    //             .intr_flags = 0
    //         },
    //         .initMode = tt::hal::spi::InitMode::ByTactility,
    //         .isMutable = false,
    //         .lock = tt::lvgl::getSyncLock() // esp_lvgl_port owns the lock for the display
    //     }
    // }
};
