#include "TactilityCore.h"
#include "hal/UnPhoneDisplayConstants.h"
#include "hx8357/disp_spi.h"
#include <driver/spi_common.h>
#include <soc/gpio_num.h>
#include <lvgl.h>

#define TAG "unphone"

// SPI
#define UNPHONE_SPI_HOST SPI2_HOST
#define UNPHONE_SPI_PIN_SCLK GPIO_NUM_39
#define UNPHONE_SPI_PIN_MOSI GPIO_NUM_40
#define UNPHONE_SPI_PIN_MISO GPIO_NUM_41
#define UNPHONE_SPI_TRANSFER_SIZE_LIMIT (UNPHONE_LCD_HORIZONTAL_RESOLUTION * UNPHONE_LCD_SPI_TRANSFER_HEIGHT * LV_COLOR_DEPTH / 8)

static bool initSpi() {
    TT_LOG_I(TAG, LOG_MESSAGE_SPI_INIT_START_FMT, UNPHONE_SPI_HOST);

    spi_bus_config_t bus_config = {
        .mosi_io_num = UNPHONE_SPI_PIN_MOSI,
        .miso_io_num = UNPHONE_SPI_PIN_MISO,
        .sclk_io_num = UNPHONE_SPI_PIN_SCLK,
        .quadwp_io_num = -1, // Quad SPI LCD driver is not yet supported
        .quadhd_io_num = -1, // Quad SPI LCD driver is not yet supported
        .data4_io_num = 0,
        .data5_io_num = 0,
        .data6_io_num = 0,
        .data7_io_num = 0,
        .data_io_default_level = false,
        .max_transfer_sz = UNPHONE_SPI_TRANSFER_SIZE_LIMIT,
        .flags = 0,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
        .intr_flags = 0
    };

    if (spi_bus_initialize(UNPHONE_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO) != ESP_OK) {
        TT_LOG_E(TAG, LOG_MESSAGE_SPI_INIT_FAILED_FMT, UNPHONE_SPI_HOST);
        return false;
    }

    return true;
}

bool unPhoneInitHardware() {
    return initSpi();
}
