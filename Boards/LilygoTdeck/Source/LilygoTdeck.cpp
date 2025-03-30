#include "Tactility/lvgl/LvglSync.h"
#include "hal/TdeckDisplay.h"
#include "hal/TdeckDisplayConstants.h"
#include "hal/TdeckKeyboard.h"
#include "hal/TdeckPower.h"
#include "hal/TdeckSdCard.h"

#include <Tactility/hal/Configuration.h>

#define TDECK_SPI_TRANSFER_SIZE_LIMIT (TDECK_LCD_HORIZONTAL_RESOLUTION * TDECK_LCD_SPI_TRANSFER_HEIGHT * (LV_COLOR_DEPTH / 8))

bool tdeckInit();

using namespace tt::hal;

extern const Configuration lilygo_tdeck = {
    .initBoot = tdeckInit,
    .createDisplay = createDisplay,
    .createKeyboard = createKeyboard,
    .sdcard = createTdeckSdCard(),
    .power = tdeck_get_power,
    .i2c = {
        i2c::Configuration {
            .name = "Internal",
            .port = I2C_NUM_0,
            .initMode = i2c::InitMode::ByTactility,
            .isMutable = false,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_18,
                .scl_io_num = GPIO_NUM_8,
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
            .isMutable = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_43,
                .scl_io_num = GPIO_NUM_44,
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
        spi::Configuration {
            .device = SPI2_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_41,
                .miso_io_num = GPIO_NUM_38,
                .sclk_io_num = GPIO_NUM_40,
                .quadwp_io_num = GPIO_NUM_NC, // Quad SPI LCD driver is not yet supported
                .quadhd_io_num = GPIO_NUM_NC, // Quad SPI LCD driver is not yet supported
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = TDECK_SPI_TRANSFER_SIZE_LIMIT,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = tt::lvgl::getSyncLock() // esp_lvgl_port owns the lock for the display
        }
    },
    .uart {
        uart::Configuration {
            .name = "Grove",
            .port = UART_NUM_1,
            .rxPin = GPIO_NUM_44,
            .txPin = GPIO_NUM_43,
            .rtsPin = GPIO_NUM_NC,
            .ctsPin = GPIO_NUM_NC,
            .rxBufferSize = 1024,
            .txBufferSize = 1024,
            .config = {
                .baud_rate = 38400,
                .data_bits = UART_DATA_8_BITS,
                .parity    = UART_PARITY_DISABLE,
                .stop_bits = UART_STOP_BITS_1,
                .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                .rx_flow_ctrl_thresh = 0,
                .source_clk = UART_SCLK_DEFAULT,
                .flags = {
                    .allow_pd = 0,
                    .backup_before_sleep = 0,
                }
            }
        }
    }
};
