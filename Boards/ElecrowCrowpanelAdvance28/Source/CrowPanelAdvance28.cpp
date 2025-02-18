#include "Tactility/lvgl/LvglSync.h"
#include "hal/CrowPanelDisplay.h"
#include "hal/CrowPanelDisplayConstants.h"
#include "hal/CrowPanelSdCard.h"

#include <Tactility/hal/Configuration.h>

#define CROWPANEL_SPI_TRANSFER_SIZE_LIMIT (CROWPANEL_LCD_HORIZONTAL_RESOLUTION * CROWPANEL_LCD_SPI_TRANSFER_HEIGHT * (CROWPANEL_LCD_BITS_PER_PIXEL / 8))

using namespace tt::hal;

extern const Configuration crowpanel_advance_28 = {
    .createDisplay = createDisplay,
    .sdcard = createSdCard(),
    .i2c = {
        i2c::Configuration {
            .name = "Internal",
            .port = I2C_NUM_0,
            .initMode = i2c::InitMode::ByTactility,
            .canReinit = false,
            .hasMutableConfiguration = false,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_15,
                .scl_io_num = GPIO_NUM_16,
                .sda_pullup_en = true,
                .scl_pullup_en = true,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        i2c::Configuration {
            .name = "External",
            .port = I2C_NUM_1,
            .initMode = i2c::InitMode::Disabled,
            .canReinit = true,
            .hasMutableConfiguration = true,
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
        // Display
        spi::Configuration {
            .device = SPI2_HOST,
            .dma = SPI_DMA_DISABLED,
            .config = {
                .mosi_io_num = GPIO_NUM_39,
                .miso_io_num = GPIO_NUM_NC,
                .sclk_io_num = GPIO_NUM_42,
                .quadwp_io_num = GPIO_NUM_NC,
                .quadhd_io_num = GPIO_NUM_NC,
                .data4_io_num = 0,
                .data5_io_num = 0,
                .data6_io_num = 0,
                .data7_io_num = 0,
                .data_io_default_level = false,
                .max_transfer_sz = CROWPANEL_SPI_TRANSFER_SIZE_LIMIT,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = spi::InitMode::ByTactility,
            .canReinit = false,
            .hasMutableConfiguration = false,
            .lock = tt::lvgl::getSyncLock() // esp_lvgl_port owns the lock for the display
        },
        // SD card
        spi::Configuration {
            .device = SPI3_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_6,
                .miso_io_num = GPIO_NUM_4,
                .sclk_io_num = GPIO_NUM_5,
                .quadwp_io_num = GPIO_NUM_NC,
                .quadhd_io_num = GPIO_NUM_NC,
                .data4_io_num = 0,
                .data5_io_num = 0,
                .data6_io_num = 0,
                .data7_io_num = 0,
                .data_io_default_level = false,
                .max_transfer_sz = 32768,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = spi::InitMode::ByTactility,
            .canReinit = false,
            .hasMutableConfiguration = false,
            .lock = nullptr // No custom lock needed
        }
    },
    .uart {},
    .gps = {}
};
