#include "devices/Display.h"
#include "devices/SdCard.h"
#include "devices/TpagerEncoder.h"
#include "devices/TpagerKeyboard.h"
#include "devices/TpagerPower.h"

#include <Tactility/hal/Configuration.h>
#include <Tactility/lvgl/LvglSync.h>
#include <Bq25896.h>
#include <Drv2605.h>

#define TPAGER_SPI_TRANSFER_SIZE_LIMIT (480 * 222 * (LV_COLOR_DEPTH / 8))

bool tpagerInit();

using namespace tt::hal;

static DeviceVector createDevices() {
    auto bq27220 = std::make_shared<Bq27220>(I2C_NUM_0);
    auto power = std::make_shared<TpagerPower>(bq27220);

    auto tca8418 = std::make_shared<Tca8418>(I2C_NUM_0);
    auto keyboard = std::make_shared<TpagerKeyboard>(tca8418);

    return std::vector<std::shared_ptr<Device>> {
        tca8418,
        std::make_shared<Bq25896>(I2C_NUM_0),
        bq27220,
        std::make_shared<Drv2605>(I2C_NUM_0),
        power,
        createTpagerSdCard(),
        createDisplay(),
        keyboard,
        std::make_shared<TpagerEncoder>()
    };
}

extern const Configuration hardwareConfiguration = {
    .initBoot = tpagerInit,
    .createDevices = createDevices,
    .i2c = {
        i2c::Configuration {
            .name = "Internal",
            .port = I2C_NUM_0,
            .initMode = i2c::InitMode::ByTactility,
            .isMutable = false,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_3,
                .scl_io_num = GPIO_NUM_2,
                .sda_pullup_en = false,
                .scl_pullup_en = false,
                .master = {
                    .clk_speed = 100'000
                },
                .clk_flags = 0
            }
        }
    },
    .spi {spi::Configuration {
        .device = SPI2_HOST,
        .dma = SPI_DMA_CH_AUTO,
        .config = {
            .mosi_io_num = GPIO_NUM_34,
            .miso_io_num = GPIO_NUM_33,
            .sclk_io_num = GPIO_NUM_35,
            .quadwp_io_num = GPIO_NUM_NC, // Quad SPI LCD driver is not yet supported
            .quadhd_io_num = GPIO_NUM_NC, // Quad SPI LCD driver is not yet supported
            .data4_io_num = GPIO_NUM_NC,
            .data5_io_num = GPIO_NUM_NC,
            .data6_io_num = GPIO_NUM_NC,
            .data7_io_num = GPIO_NUM_NC,
            .data_io_default_level = false,
            .max_transfer_sz = TPAGER_SPI_TRANSFER_SIZE_LIMIT,
            .flags = 0,
            .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
            .intr_flags = 0
        },
        .initMode = spi::InitMode::ByTactility,
        .isMutable = false,
        .lock = tt::lvgl::getSyncLock() // esp_lvgl_port owns the lock for the display
    }},
    .uart {uart::Configuration {
        .name = "Internal",
        .port = UART_NUM_1,
        .rxPin = GPIO_NUM_4,
        .txPin = GPIO_NUM_12,
        .rtsPin = GPIO_NUM_NC,
        .ctsPin = GPIO_NUM_NC,
        .rxBufferSize = 1024,
        .txBufferSize = 1024,
        .config = {
            .baud_rate = 38400,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 0,
            .source_clk = UART_SCLK_DEFAULT,
            .flags = {
                .allow_pd = 0,
                .backup_before_sleep = 0,
            }
        }
    }}
};
