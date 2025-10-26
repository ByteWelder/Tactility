#include "CYD2432S032C.h"
#include "Tactility/lvgl/LvglSync.h"
#include "devices/Display.h"
#include "devices/SdCard.h"

#include <Tactility/kernel/SystemEvents.h>

#include <PwmBacklight.h>

bool initBoot() {
    if (!driver::pwmbacklight::init(GPIO_NUM_27)) {
        return false;
    }

    // This display has a weird glitch with gamma during boot, which results in uneven dark gray colours.
    // Setting gamma curve index to 0 doesn't work at boot for an unknown reason, so we set the curve index to 1:
    tt::kernel::subscribeSystemEvent(tt::kernel::SystemEvent::BootSplash, [](auto) {
        auto display = tt::hal::findFirstDevice<tt::hal::display::DisplayDevice>(tt::hal::Device::Type::Display);
        assert(display != nullptr);
        tt::lvgl::lock(portMAX_DELAY);
        display->setGammaCurve(1U);
        tt::lvgl::unlock();
    });

    return true;
}

static tt::hal::DeviceVector createDevices() {
    return {
        createDisplay(),
        createSdCard()
    };
}

const tt::hal::Configuration cyd_2432S032c_config = {
    .initBoot = initBoot,
    .createDevices = createDevices,
    .i2c = {
        tt::hal::i2c::Configuration {
            .name = "Internal",
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
            .initMode = tt::hal::spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = nullptr
        },
    }
};
