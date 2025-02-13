#include "M5stackCoreS3.h"
#include "InitBoot.h"
#include "Tactility/lvgl/LvglSync.h"
#include "hal/CoreS3Display.h"
#include "hal/CoreS3DisplayConstants.h"
#include "hal/CoreS3Power.h"
#include "hal/CoreS3SdCard.h"

#define CORES3_TRANSACTION_SIZE (CORES3_LCD_DRAW_BUFFER_SIZE * LV_COLOR_DEPTH / 8)

const tt::hal::Configuration m5stack_cores3 = {
    .initBoot = initBoot,
    .createDisplay = createDisplay,
    .sdcard = createSdCard(),
    .power = createPower,
    .i2c = {
        tt::hal::i2c::Configuration {
            .name = "Internal",
            .port = I2C_NUM_0,
            .initMode = tt::hal::i2c::InitMode::ByTactility,
            .canReinit = false,
            .hasMutableConfiguration = false,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_12,
                .scl_io_num = GPIO_NUM_11,
                .sda_pullup_en = true,
                .scl_pullup_en = true,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        tt::hal::i2c::Configuration {
            .name = "External", // Grove
            .port = I2C_NUM_1,
            .initMode = tt::hal::i2c::InitMode::ByTactility,
            .canReinit = true,
            .hasMutableConfiguration = true,
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
        }
    },
    .spi {
        tt::hal::spi::Configuration {
            .device = SPI3_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_37,
                .miso_io_num = GPIO_NUM_35,
                .sclk_io_num = GPIO_NUM_36,
                .data2_io_num = GPIO_NUM_NC,
                .data3_io_num = GPIO_NUM_NC,
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = CORES3_TRANSACTION_SIZE,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = tt::hal::spi::InitMode::ByTactility,
            .canReinit = false,
            .hasMutableConfiguration = false,
            .lock = tt::lvgl::getSyncLock() // esp_lvgl_port owns the lock for the display
        }
    }
};
