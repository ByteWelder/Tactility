#include "PwmBacklight.h"
#include "Tactility/lvgl/LvglSync.h"
#include "hal/CrowPanelDisplay.h"
#include "hal/CrowPanelDisplayConstants.h"
#include "hal/CrowPanelSdCard.h"

#include <Tactility/hal/Configuration.h>

#define CROWPANEL_SPI_TRANSFER_SIZE_LIMIT (CROWPANEL_LCD_HORIZONTAL_RESOLUTION * CROWPANEL_LCD_SPI_TRANSFER_HEIGHT * (LV_COLOR_DEPTH / 8))

using namespace tt::hal;

bool initBoot() {
    return driver::pwmbacklight::init(GPIO_NUM_38);
}

extern const Configuration crowpanel_advance_28 = {
    .initBoot = initBoot,
    .createDisplay = createDisplay,
    .sdcard = createSdCard(),
    .i2c = {
        // There is only 1 (internal for touch, and also serves as "I2C-OUT" port)
        // Note: You could repurpose 1 or more UART interfaces as I2C interfaces
        i2c::Configuration {
            .name = "Main",
            .port = I2C_NUM_0,
            .initMode = i2c::InitMode::ByTactility,
            .isMutable = false,
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
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = CROWPANEL_SPI_TRANSFER_SIZE_LIMIT,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = spi::InitMode::ByTactility,
            .isMutable = false,
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
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = 32768,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = nullptr // No custom lock needed
        }
    },
    .uart {
        // "UART0-IN"
        uart::Configuration {
            .name = "UART0",
            .port = UART_NUM_1,
            .rxPin = GPIO_NUM_44,
            .txPin = GPIO_NUM_43,
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
        // "UART1-OUT"
        uart::Configuration {
            .name = "UART1",
            .port = UART_NUM_2,
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
