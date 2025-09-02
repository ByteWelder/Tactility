#include "Tactility/lvgl/LvglSync.h"
#include "UnPhoneFeatures.h"
#include "Xpt2046Power.h"
#include "devices/Hx8357Display.h"
#include "devices/SdCard.h"
#include <Tactility/hal/Configuration.h>

#define UNPHONE_SPI_TRANSFER_SIZE_LIMIT (UNPHONE_LCD_HORIZONTAL_RESOLUTION * UNPHONE_LCD_SPI_TRANSFER_HEIGHT * LV_COLOR_DEPTH / 8)

bool initBoot();

static tt::hal::DeviceVector createDevices() {
    return {
        std::make_shared<Xpt2046Power>(),
        createDisplay(),
        createSdCard()
    };
}

extern const tt::hal::Configuration unPhone = {
    .initBoot = initBoot,
    .createDevices = createDevices,
    .i2c = {
        tt::hal::i2c::Configuration {
            .name = "Internal",
            .port = I2C_NUM_0,
            .initMode = tt::hal::i2c::InitMode::ByTactility,
            .isMutable = false,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_3,
                .scl_io_num = GPIO_NUM_4,
                .sda_pullup_en = true,
                .scl_pullup_en = true,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        tt::hal::i2c::Configuration {
            .name = "Unused",
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
                .mosi_io_num = GPIO_NUM_40,
                .miso_io_num = GPIO_NUM_41,
                .sclk_io_num = GPIO_NUM_39,
                .quadwp_io_num = -1, // Quad SPI LCD driver is not yet supported
                .quadhd_io_num = -1, // Quad SPI LCD driver is not yet supported
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = UNPHONE_SPI_TRANSFER_SIZE_LIMIT,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = tt::hal::spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = tt::lvgl::getSyncLock() // esp_lvgl_port owns the lock for the display
        }
    }
};
