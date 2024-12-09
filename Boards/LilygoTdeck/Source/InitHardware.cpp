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

bool init_power_adc();

static bool init_spi() {
    spi_bus_config_t bus_config = {
        .mosi_io_num = TDECK_SPI_PIN_MOSI,
        .miso_io_num = TDECK_SPI_PIN_MISO,
        .sclk_io_num = TDECK_SPI_PIN_SCLK,
        .quadwp_io_num = -1, // Quad SPI LCD driver is not yet supported
        .quadhd_io_num = -1, // Quad SPI LCD driver is not yet supported
        .max_transfer_sz = TDECK_SPI_TRANSFER_SIZE_LIMIT,
    };

    return spi_bus_initialize(TDECK_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO) == ESP_OK;
}

bool tdeck_init_hardware() {
    TT_LOG_I(TAG, "Init SPI");

    if (!init_spi()) {
        TT_LOG_E(TAG, "Init SPI failed");
        return false;
    }

    TT_LOG_I(TAG, "Init ADC");
    if (!init_power_adc()) {
        TT_LOG_W(TAG, "ADC init failed");
    }

    return true;
}