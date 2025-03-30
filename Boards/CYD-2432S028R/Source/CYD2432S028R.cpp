#include "PwmBacklight.h"
#include "Tactility/lvgl/LvglSync.h"
#include "hal/YellowDisplay.h"
#include "hal/YellowDisplayConstants.h"
#include "hal/YellowSDCard.h"

#include <Tactility/hal/Configuration.h>

#define YELLOW_SPI_TRANSFER_SIZE_LIMIT (YELLOW_LCD_HORIZONTAL_RESOLUTION * YELLOW_LCD_SPI_TRANSFER_HEIGHT * (LV_COLOR_DEPTH / 8))

using namespace tt::hal;

bool initBoot() {
    return driver::pwmbacklight::init(GPIO_NUM_21);
}

extern const Configuration cyd_2432s028r_config = {
    .initBoot = initBoot,
    .createDisplay = createDisplay,
    .sdcard = createSdCard(),
    .power = nullptr,
    .spi {
        // Display (ILI9341 on SPI2_HOST)
        spi::Configuration {
            .device = SPI2_HOST,
            .dma = SPI_DMA_DISABLED,
            .config = {
                .mosi_io_num = GPIO_NUM_13,
                .miso_io_num = GPIO_NUM_12,
                .sclk_io_num = GPIO_NUM_14,
                .quadwp_io_num = GPIO_NUM_NC,
                .quadhd_io_num = GPIO_NUM_NC,
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = YELLOW_SPI_TRANSFER_SIZE_LIMIT,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = tt::lvgl::getSyncLock()
        },
        // Touch (XPT2046 on SPI1_HOST)
        spi::Configuration {
            .device = SPI1_HOST,
            .dma = SPI_DMA_DISABLED,
            .config = {
                .mosi_io_num = GPIO_NUM_32,
                .miso_io_num = GPIO_NUM_39,
                .sclk_io_num = GPIO_NUM_25,
                .quadwp_io_num = GPIO_NUM_NC,
                .quadhd_io_num = GPIO_NUM_NC,
                .data4_io_num = GPIO_NUM_NC,
                .data5_io_num = GPIO_NUM_NC,
                .data6_io_num = GPIO_NUM_NC,
                .data7_io_num = GPIO_NUM_NC,
                .data_io_default_level = false,
                .max_transfer_sz = 4096,
                .flags = 0,
                .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
                .intr_flags = 0
            },
            .initMode = spi::InitMode::ByTactility,
            .isMutable = false,
            .lock = tt::lvgl::getSyncLock()
        },
        // SD Card (SPI3_HOST)
        spi::Configuration {
            .device = SPI3_HOST,
            .dma = SPI_DMA_CH_AUTO,
            .config = {
                .mosi_io_num = GPIO_NUM_23,
                .miso_io_num = GPIO_NUM_19,
                .sclk_io_num = GPIO_NUM_18,
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
            .lock = nullptr
        }
    },
    .uart {
        // UART0 (corrected for CYD's USB serial)
        uart::Configuration {
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
    /*
    // Optional: RGB LED (uncomment to enable)
    .gpio = {
        { GPIO_NUM_4, GPIO_MODE_OUTPUT, false },  // Red
        { GPIO_NUM_16, GPIO_MODE_OUTPUT, false }, // Green
        { GPIO_NUM_17, GPIO_MODE_OUTPUT, false }, // Blue
        { GPIO_NUM_27, GPIO_MODE_INPUT, true },   // Extended GPIO (CN1)
        { GPIO_NUM_35, GPIO_MODE_INPUT, true },   // Extended GPIO (P3)
        { GPIO_NUM_22, GPIO_MODE_INPUT, true }    // Extended GPIO (P3)
    },
    // Optional: CDS Light Sensor (uncomment to enable)
    .adc = {
        adc::Configuration {
            .unit = ADC_UNIT_1,
            .channel = ADC1_CHANNEL_6,  // GPIO 34
            .atten = ADC_ATTEN_DB_11,
            .bitwidth = ADC_WIDTH_BIT_12
        }
    },
    // Optional: Speaker PWM (uncomment to enable)
    .pwm = {
        pwm::Configuration {
            .channel = LEDC_CHANNEL_0,
            .pin = GPIO_NUM_26,
            .freq_hz = 1000,
            .timer = LEDC_TIMER_0
        }
    }
    */
};
