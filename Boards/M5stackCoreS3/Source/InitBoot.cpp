#include <driver/i2c.h>
#include <driver/spi_master.h>
#include <intr_types.h>
#include "Log.h"
#include "hal/CoreS3DisplayConstants.h"
#include "hal/i2c/I2c.h"
#include "CoreS3Constants.h"

#define TAG "core2"

#define CORES3_SPI2_PIN_SCLK GPIO_NUM_36
#define CORES3_SPI2_PIN_MOSI GPIO_NUM_37
#define CORES3_SPI2_PIN_MISO GPIO_NUM_35

/**
 * For details see https://github.com/espressif/esp-bsp/blob/master/bsp/m5stack_core_s3/m5stack_core_s3.c
 */
static bool initSpi3() {
    TT_LOG_I(TAG, LOG_MESSAGE_SPI_INIT_START_FMT, SPI3_HOST);
    const spi_bus_config_t bus_config = {
        .mosi_io_num = CORES3_SPI2_PIN_MOSI,
        .miso_io_num = CORES3_SPI2_PIN_MISO,
        .sclk_io_num = CORES3_SPI2_PIN_SCLK,
        .data2_io_num = GPIO_NUM_NC,
        .data3_io_num = GPIO_NUM_NC,
        .data4_io_num = GPIO_NUM_NC,
        .data5_io_num = GPIO_NUM_NC,
        .data6_io_num = GPIO_NUM_NC,
        .data7_io_num = GPIO_NUM_NC,
        .max_transfer_sz = CORES3_LCD_DRAW_BUFFER_SIZE,
        .flags = 0,
        .isr_cpu_id = INTR_CPU_ID_AUTO,
        .intr_flags = 0
    };

    if (spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO) != ESP_OK) {
        TT_LOG_E(TAG, LOG_MESSAGE_SPI_INIT_FAILED_FMT, SPI3_HOST);
        return false;
    }

    return true;
}

/**
 * For details see https://github.com/espressif/esp-bsp/blob/master/bsp/m5stack_core_s3/m5stack_core_s3.c
 */
bool initGpioExpander() {
    TT_LOG_I(TAG, "Init AW9523 GPIO expander");
    uint8_t aw9523_P0 = 0b10;
    uint8_t aw9523_P1 = 0b10100000;

    // Enable LCD
    aw9523_P1 |= (1 << 1);

    // Enable touch
    aw9523_P0 |= (1);

    // SD card
    aw9523_P0 |= (1 << 4);

    if (!tt::hal::i2c::masterWrite(I2C_NUM_0, AW9523_ADDRESS, 0x02, &aw9523_P0, 1, 1000)) {
        TT_LOG_E(TAG, "Failed to enable LCD");
        return false;
    }

    if (!tt::hal::i2c::masterWrite(I2C_NUM_0, AW9523_ADDRESS, 0x03, &aw9523_P1, 1, 1000)) {
        TT_LOG_E(TAG, "Failed to enable touch");
        return false;
    }

    return true;
}

bool initPowerControl() {
    TT_LOG_I(TAG, "Init power control");

    uint8_t sdcard_3v3 = 0b00011100;
    // TODO: Refactor to use Axp2101 class with https://github.com/m5stack/M5Unified/blob/b8cfec7fed046242da7f7b8024a4e92004a51ff7/src/utility/AXP2101_Class.cpp#L62
    if (!tt::hal::i2c::masterWrite(I2C_NUM_0, AXP2101_ADDRESS, 0x95, &sdcard_3v3, 1, 1000)) {
        TT_LOG_E(TAG, "Failed to enable SD card");
        return false;
    }

    // TODO: Refactor to use Axp2102 class with https://github.com/m5stack/M5Unified/blob/b8cfec7fed046242da7f7b8024a4e92004a51ff7/src/utility/AXP2101_Class.cpp#L42
    uint8_t axp_dld01_enable = 0xBF; // For backlight
    if (!tt::hal::i2c::masterWrite(I2C_NUM_0, AXP2101_ADDRESS, 0x90, &axp_dld01_enable, 1, 1000)) {
        TT_LOG_E(TAG, "Failed to enable display backlight");
        return false;
    }

    // TODO: Refactor to use Axp2101 class with https://github.com/m5stack/M5Unified/blob/b8cfec7fed046242da7f7b8024a4e92004a51ff7/src/utility/AXP2101_Class.cpp#L62
    uint8_t axp_dld01_voltage = 0b00011000; // For backlight
    if (!tt::hal::i2c::masterWrite(I2C_NUM_0, AXP2101_ADDRESS, 0x99, &axp_dld01_voltage, 1, 1000)) {
        TT_LOG_E(TAG, "Failed to set display backlight voltage");
        return false;
    }

    return true;
}

bool initBoot() {
    TT_LOG_I(TAG, "initBoot");
    return initPowerControl() && initGpioExpander() && initSpi3();
}