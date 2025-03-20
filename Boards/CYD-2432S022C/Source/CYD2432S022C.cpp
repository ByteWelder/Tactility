#include "CYD2432S022C.h"
#include "hal/ST7789Display.h"
#include "hal/YellowSDCard.h"
#include "hal/CYD2432S022CConstants.h"
#include <Tactility/lvgl/LvglSync.h>

// Forward declaration of the display factory function
std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();

bool init_boot() {
    // No backlight PWM for this board (not specified in the schematic)
    return true;
}

const tt::hal::Configuration cyd_2432s022c_config = {
    .initBoot = init_boot,
    .createDisplay = createDisplay,
    .sdcard = createYellowSDCard(),
    .power = nullptr,
    .i2c = {
        // I2C for CST820 touch
        tt::hal::i2c::Configuration {
            .name = "First",
            .port = CYD_2432S022C_TOUCH_I2C_PORT,
            .initMode = tt::hal::i2c::InitMode::ByTactility,
            .isMutable = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = CYD_2432S022C_TOUCH_I2C_SDA,
                .scl_io_num = CYD_2432S022C_TOUCH_I2C_SCL,
                .sda_pullup_en = GPIO_PULLUP_ENABLE,
                .scl_pullup_en = GPIO_PULLUP_ENABLE,
                .master = {
                    .clk_speed = CYD_2432S022C_TOUCH_I2C_SPEED
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
        // SPI for SD card
        tt::hal::spi::Configuration {
            .device = CYD_2432S022C_SDCARD_SPI_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = CYD_2432S022C_SDCARD_PIN_MOSI,
                .miso_io_num = CYD_2432S022C_SDCARD_PIN_MISO,
                .sclk_io_num = CYD_2432S022C_SDCARD_PIN_SCLK,
                .quadwp_io_num = GPIO_NUM_NC,
                .quadhd_io_num = GPIO_NUM_NC,
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = CYD_2432S022C_SDCARD_SPI_MAX_TRANSFER_SIZE,
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
