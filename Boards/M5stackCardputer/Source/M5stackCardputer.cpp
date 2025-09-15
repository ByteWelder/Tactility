#include "M5stackCardputer.h"
#include "InitBoot.h"
#include "devices/Display.h"
#include "devices/SdCard.h"
#include "devices/CardputerEncoder.h"
#include "devices/CardputerKeyboard.h"
#include "devices/CardputerPower.h"

#include <lvgl.h>
#include <Tactility/lvgl/LvglSync.h>

#define SPI_TRANSFER_SIZE_LIMIT (LCD_DRAW_BUFFER_SIZE * LV_COLOR_DEPTH / 8)

using namespace tt::hal;

static DeviceVector createDevices() {
    return {
        createSdCard(),
        createDisplay(),
        std::make_shared<CardputerKeyboard>(),
        std::make_shared<CardputerEncoder>(),
        std::make_shared<CardputerPower>()
    };
}

extern const Configuration m5stack_cardputer = {
    .initBoot = initBoot,
    .uiScale = UiScale::Smallest,
    .createDevices = createDevices,
    .i2c {
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
    },
    .spi {
        // Display
        spi::Configuration {
            .device = SPI2_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_35,
                .miso_io_num = GPIO_NUM_NC,
                .sclk_io_num = GPIO_NUM_36,
                .quadwp_io_num = GPIO_NUM_NC,
                .quadhd_io_num = GPIO_NUM_NC,
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = SPI_TRANSFER_SIZE_LIMIT,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = tt::lvgl::getSyncLock()
        },
        // SDCard
        spi::Configuration {
            .device = SPI3_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_14,
                .miso_io_num = GPIO_NUM_39,
                .sclk_io_num = GPIO_NUM_40,
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
            .initMode = spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = nullptr
        },

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
        }
    }
};
