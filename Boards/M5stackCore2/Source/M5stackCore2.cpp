#include "M5stackCore2.h"
#include "InitBoot.h"
#include "Tactility/lvgl/LvglSync.h"
#include "hal/Core2Display.h"
#include "hal/Core2DisplayConstants.h"
#include "hal/Core2Power.h"
#include "hal/Core2SdCard.h"

#define CORE2_SPI_TRANSFER_SIZE_LIMIT (CORE2_LCD_DRAW_BUFFER_SIZE * LV_COLOR_DEPTH / 8)

extern const tt::hal::Configuration m5stack_core2 = {
    .initBoot = initBoot,
    .createDisplay = createDisplay,
    .sdcard = createSdCard(),
    .power = createPower,
    .i2c = {
        tt::hal::i2c::Configuration {
            .name = "Internal",
            .port = I2C_NUM_0,
            .initMode = tt::hal::i2c::InitMode::ByTactility,
            .canReinit = false, // Might be set to try after trying out what it does AXP and screen
            .hasMutableConfiguration = false,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_21,
                .scl_io_num = GPIO_NUM_22,
                .sda_pullup_en = true,
                .scl_pullup_en = true,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        tt::hal::i2c::Configuration {
            .name = "External", // (Grove)
            .port = I2C_NUM_1,
            .initMode = tt::hal::i2c::InitMode::ByTactility,
            .canReinit = true,
            .hasMutableConfiguration = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_32,
                .scl_io_num = GPIO_NUM_33,
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
            .device = SPI2_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_23,
                .miso_io_num = GPIO_NUM_38,
                .sclk_io_num = GPIO_NUM_18,
                .quadwp_io_num = -1, // Quad SPI LCD driver is not yet supported
                .quadhd_io_num = -1, // Quad SPI LCD driver is not yet supported
                .data4_io_num = 0,
                .data5_io_num = 0,
                .data6_io_num = 0,
                .data7_io_num = 0,
                .data_io_default_level = false,
                .max_transfer_sz = CORE2_SPI_TRANSFER_SIZE_LIMIT,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = tt::hal::spi::InitMode::ByTactility,
            .canReinit = false,
            .hasMutableConfiguration = false,
            .lock = tt::lvgl::getLvglSyncLockable() // esp_lvgl_port owns the lock for the display
        }
    }
};
