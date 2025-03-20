#include "CYD2432S022C.h"
#include "hal/ST7789Display.h"
#include "hal/YellowSDCard.h"
#include <Tactility/lvgl/LvglSync.h>

// Forward declaration of the display factory function
std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();

bool init_boot() {
    // No backlight PWM for this board (not specified in the schematic)
    return true;
}

const tt::hal::Configuration cyd_2432s022c_config = {
    .initBoot = init_boot,
    .createDisplay = create_display,
    .sdcard = createYellowSDCard(),
    .power = nullptr,
    .i2c = {
        // I2C for CST820 touch
        tt::hal::i2c::Configuration {
            .name = "First",
            .port = I2C_NUM_0,
            .initMode = tt::hal::i2c::InitMode::ByTactility,
            .isMutable = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_21,  // From schematic
                .scl_io_num = GPIO_NUM_22,  // From schematic
                .sda_pullup_en = GPIO_PULLUP_ENABLE,
                .scl_pullup_en = GPIO_PULLUP_ENABLE,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        // Second I2C port (disabled)
        tt::hal::i2c::Configuration {
            .name = "Second",
            .port = I2C_NUM_1,
            .initMode = tt::hal::i2c::InitMode::Disabled,
            .isMutable = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_NC,
                .scl_io_num = GPIO_NUM_NC,
                .sda_pullup_en = GPIO_PULLUP_DISABLE,
                .scl_pullup_en = GPIO_PULLUP_DISABLE,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        }
    },
    .spi = {
        // SPI for SD card (no SPI for display since it's 8-bit parallel)
        tt::hal::spi::Configuration {
            .device = SPI3_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_23,  // From schematic
                .miso_io_num = GPIO_NUM_19,  // From schematic
                .sclk_io_num = GPIO_NUM_18,  // From schematic
                .quadwp_io_num = GPIO_NUM_NC,
                .quadhd_io_num = GPIO_NUM_NC,
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
        // Second SPI (disabled)
        tt::hal::spi::Configuration {
            .device = SPI2_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_NC,
                .miso_io_num = GPIO_NUM_NC,
                .sclk_io_num = GPIO_NUM_NC,
                .quadwp_io_num = GPIO_NUM_NC,
                .quadhd_io_num = GPIO_NUM_NC,
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
            .initMode = tt::hal::spi::InitMode::Disabled,
            .isMutable = false,
            .lock = nullptr
        }
    }
};
