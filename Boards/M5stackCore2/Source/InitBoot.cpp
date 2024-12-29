#include <driver/i2c.h>
#include <driver/spi_master.h>
#include <intr_types.h>
#include "Log.h"
#include "hal/Core2DisplayConstants.h"
#include "M5Unified/src/utility/AXP192_Class.hpp"

#define TAG "core2"

#define CORE2_SPI2_PIN_SCLK GPIO_NUM_18
#define CORE2_SPI2_PIN_MOSI GPIO_NUM_23
#define CORE2_SPI2_PIN_MISO GPIO_NUM_38

static bool initSpi2() {
    TT_LOG_I(TAG, LOG_MESSAGE_SPI_INIT_START_FMT, SPI2_HOST);
    const spi_bus_config_t bus_config = {
        .mosi_io_num = CORE2_SPI2_PIN_MOSI,
        .miso_io_num = CORE2_SPI2_PIN_MISO,
        .sclk_io_num = CORE2_SPI2_PIN_SCLK,
        .data2_io_num = GPIO_NUM_NC,
        .data3_io_num = GPIO_NUM_NC,
        .data4_io_num = GPIO_NUM_NC,
        .data5_io_num = GPIO_NUM_NC,
        .data6_io_num = GPIO_NUM_NC,
        .data7_io_num = GPIO_NUM_NC,
        .max_transfer_sz = CORE2_LCD_DRAW_BUFFER_SIZE,
        .flags = 0,
        .isr_cpu_id = INTR_CPU_ID_AUTO,
        .intr_flags = 0
    };

    if (spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO) != ESP_OK) {
        TT_LOG_E(TAG, LOG_MESSAGE_SPI_INIT_FAILED_FMT, SPI2_HOST);
        return false;
    }

    return true;
}

bool initAxp() {
    m5::I2C_Class i2c;
    m5::AXP192_Class axpDevice(0x34, 400000, &i2c);

    if (!i2c.begin(I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22)) {
        TT_LOG_E(TAG, "I2C init failed");
        return false;
    }

    if (!axpDevice.begin()) {
        TT_LOG_E(TAG, "AXP init failed");
        return false;
    }

    axpDevice.setLDO2(3300); // LCD + SD peripheral power supply
    axpDevice.setLDO3(0); // VIB_MOTOR STOP
    axpDevice.setGPIO2(false); // SPEAKER STOP
    axpDevice.writeRegister8(0x9A, 255); // PWM 255 (LED OFF)
    axpDevice.writeRegister8(0x92, 0x02); // GPIO1 PWM
    axpDevice.setChargeCurrent(390); // Core2 battery = 390mAh
    axpDevice.setDCDC3(3300);
    i2c.stop();

    return true;
}

bool initBoot() {
    TT_LOG_I(TAG, "initBoot");
    return initAxp() && initSpi2();
}