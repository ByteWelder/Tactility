#include "TactilityCore.h"
#include "hal/TdeckDisplayConstants.h"
#include <driver/spi_common.h>
#include <soc/gpio_num.h>

#define TAG "tdeck"

// SPI
#define TDECK_SPI_HOST SPI2_HOST
#define TDECK_SPI_PIN_SCLK GPIO_NUM_40
#define TDECK_SPI_PIN_MOSI GPIO_NUM_41
#define TDECK_SPI_PIN_MISO GPIO_NUM_38
#define TDECK_SPI_TRANSFER_SIZE_LIMIT (TDECK_LCD_HORIZONTAL_RESOLUTION * TDECK_LCD_SPI_TRANSFER_HEIGHT * (TDECK_LCD_BITS_PER_PIXEL / 8))

#define TDECK_LCD_BACKLIGHT_LEDC_TIMER LEDC_TIMER_0
#define TDECK_LCD_BACKLIGHT_LEDC_MODE LEDC_LOW_SPEED_MODE
#define TDECK_LCD_BACKLIGHT_LEDC_CHANNEL LEDC_CHANNEL_0
#define TDECK_LCD_BACKLIGHT_LEDC_DUTY_RES LEDC_TIMER_8_BIT
#define TDECK_LCD_BACKLIGHT_LEDC_FREQUENCY (4000)

static bool init_spi() {
    TT_LOG_I(TAG, LOG_MESSAGE_SPI_INIT_START_FMT, TDECK_SPI_HOST);

    spi_bus_config_t bus_config = {
        .mosi_io_num = TDECK_SPI_PIN_MOSI,
        .miso_io_num = TDECK_SPI_PIN_MISO,
        .sclk_io_num = TDECK_SPI_PIN_SCLK,
        .quadwp_io_num = -1, // Quad SPI LCD driver is not yet supported
        .quadhd_io_num = -1, // Quad SPI LCD driver is not yet supported
        .data4_io_num = 0,
        .data5_io_num = 0,
        .data6_io_num = 0,
        .data7_io_num = 0,
        .data_io_default_level = false,
        .max_transfer_sz = TDECK_SPI_TRANSFER_SIZE_LIMIT,
        .flags = 0,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
        .intr_flags = 0
    };

    if (spi_bus_initialize(TDECK_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO) != ESP_OK) {
        TT_LOG_E(TAG, LOG_MESSAGE_SPI_INIT_FAILED_FMT, TDECK_SPI_HOST);
        return false;
    }

    return true;
}

bool tdeck_init_hardware() {
    return init_spi();
}
