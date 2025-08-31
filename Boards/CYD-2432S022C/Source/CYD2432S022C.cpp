#include "CYD2432S022C.h"
#include "hal/YellowDisplay.h"
#include "hal/YellowSDCard.h"
#include "hal/CYD2432S022CConstants.h"
#include <Tactility/lvgl/LvglSync.h>
#include "esp_log.h"
#include <PwmBacklight.h>

#define TAG "CYD2432S022C"

static bool initBoot() {
    // Initialize PWM backlight before creating display
    driver::pwmbacklight::init(CYD_2432S022C_LCD_PIN_BACKLIGHT, 40000); // Recommended 40 kHz
    ESP_LOGI("YellowDisplay", "Setting backlight duty to 255");
    driver::pwmbacklight::setBacklightDuty(255);
    return true;
}

const tt::hal::Configuration cyd_2432s022c_config = {
    .initBoot = initBoot,
    .createDisplay = createDisplay,
    .sdcard = createYellowSDCard(),
    .power = nullptr,
    .i2c = {
        tt::hal::i2c::Configuration {
            .name = "Touch",
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
        }
    },
    .spi = {
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
        }
    },
    .uart = {
        tt::hal::uart::Configuration {
            .name = "UART0",
            .port = UART_NUM_0,
            .rxPin = GPIO_NUM_3,
            .txPin = GPIO_NUM_1,
            .rtsPin = GPIO_NUM_NC,
            .ctsPin = GPIO_NUM_NC,
            .rxBufferSize = 1024,
            .txBufferSize = 1024,
            .config = {
                .baud_rate = 115200,
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
        }
    }
};
