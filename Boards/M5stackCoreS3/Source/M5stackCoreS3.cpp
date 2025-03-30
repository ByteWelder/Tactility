#include "M5stackCoreS3.h"
#include "InitBoot.h"
#include "hal/CoreS3Display.h"
#include "hal/CoreS3DisplayConstants.h"
#include "hal/CoreS3Power.h"
#include "hal/CoreS3SdCard.h"

#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/hal/uart/Uart.h>

#define CORES3_TRANSACTION_SIZE (CORES3_LCD_DRAW_BUFFER_SIZE * LV_COLOR_DEPTH / 8)

using namespace tt::hal;

const Configuration m5stack_cores3 = {
    .initBoot = initBoot,
    .createDisplay = createDisplay,
    .sdcard = createSdCard(),
    .power = createPower,
    .i2c = {
        i2c::Configuration {
            .name = "Internal",
            .port = I2C_NUM_0,
            .initMode = i2c::InitMode::ByTactility,
            .isMutable = false,
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
        i2c::Configuration {
            .name = "Port A", // Grove
            .port = I2C_NUM_1,
            .initMode = i2c::InitMode::Disabled,
            .isMutable = true,
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
        },
        i2c::Configuration {
            .name = "Port B", // Grove
            .port = I2C_NUM_1,
            .initMode = i2c::InitMode::Disabled,
            .isMutable = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_9,
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
            .name = "Port C", // Grove
            .port = I2C_NUM_1,
            .initMode = i2c::InitMode::Disabled,
            .isMutable = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_18,
                .scl_io_num = GPIO_NUM_17,
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
        spi::Configuration {
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
            .initMode = spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = tt::lvgl::getSyncLock() // esp_lvgl_port owns the lock for the display
        }
    },
    .uart {
        uart::Configuration {
            .name = "Port A",
            .port = UART_NUM_1,
            .rxPin = GPIO_NUM_2,
            .txPin = GPIO_NUM_1,
            .rtsPin = GPIO_NUM_NC,
            .ctsPin = GPIO_NUM_NC,
            .rxBufferSize = 1024,
            .txBufferSize = 1024,
            .config = {
                .baud_rate = 115200,
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
        },
        uart::Configuration {
            .name = "Port B",
            .port = UART_NUM_1,
            .rxPin = GPIO_NUM_9,
            .txPin = GPIO_NUM_8,
            .rtsPin = GPIO_NUM_NC,
            .ctsPin = GPIO_NUM_NC,
            .rxBufferSize = 1024,
            .txBufferSize = 1024,
            .config = {
                .baud_rate = 115200,
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
        },
        uart::Configuration {
            .name = "Port C",
            .port = UART_NUM_1,
            .rxPin = GPIO_NUM_18,
            .txPin = GPIO_NUM_17,
            .rtsPin = GPIO_NUM_NC,
            .ctsPin = GPIO_NUM_NC,
            .rxBufferSize = 1024,
            .txBufferSize = 1024,
            .config = {
                .baud_rate = 115200,
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
