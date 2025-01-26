#include <driver/i2c.h>
#include <driver/spi_master.h>
#include <esp_intr_types.h>
#include "Log.h"
#include "hal/Core2DisplayConstants.h"
#include "axp192/axp192.h"
#include "hal/i2c/I2c.h"
#include "CoreDefines.h"

#define TAG "core2"

#define CORE2_SPI2_PIN_SCLK GPIO_NUM_18
#define CORE2_SPI2_PIN_MOSI GPIO_NUM_23
#define CORE2_SPI2_PIN_MISO GPIO_NUM_38

axp192_t axpDevice;

static int32_t axpI2cRead(TT_UNUSED void* handle, uint8_t address, uint8_t reg, uint8_t* buffer, uint16_t size) {
    return tt::hal::i2c::masterReadRegister(I2C_NUM_0, address, reg, buffer, size, 50 / portTICK_PERIOD_MS);
}

static int32_t axpI2cWrite(TT_UNUSED void* handle, uint8_t address, uint8_t reg, const uint8_t* buffer, uint16_t size) {
    return tt::hal::i2c::masterWriteRegister(I2C_NUM_0, address, reg, buffer, size, 50 / portTICK_PERIOD_MS);
}

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
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
        .intr_flags = 0
    };

    if (spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO) != ESP_OK) {
        TT_LOG_E(TAG, LOG_MESSAGE_SPI_INIT_FAILED_FMT, SPI2_HOST);
        return false;
    }

    return true;
}

bool initAxp() {
    axpDevice.read = axpI2cRead;
    axpDevice.write = axpI2cWrite;

    axp192_ioctl(&axpDevice, AXP192_LDO2_SET_VOLTAGE, 3300); // LCD + SD
    axp192_ioctl(&axpDevice, AXP192_LDO3_SET_VOLTAGE, 0); // VIB_MOTOR STOP
    axp192_ioctl(&axpDevice, AXP192_DCDC3_SET_VOLTAGE, 3300);

    axp192_ioctl(&axpDevice, AXP192_LDO2_ENABLE);
    axp192_ioctl(&axpDevice, AXP192_LDO3_DISABLE);
    axp192_ioctl(&axpDevice, AXP192_DCDC3_ENABLE);

    axp192_write(&axpDevice, AXP192_PWM1_DUTY_CYCLE_2, 255); // PWM 255 (LED OFF)
    axp192_write(&axpDevice, AXP192_GPIO1_CONTROL, 0x02); // GPIO1 PWM
    // TODO: We could charge at 390mA according to the M5Unified code, but the AXP driver in M5Unified limits to 132mA, so it's unclear what the AXP supports.

    return true;
}

bool initBoot() {
    TT_LOG_I(TAG, "initBoot");
    return initAxp() && initSpi2();
}