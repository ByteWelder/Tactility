#include "axp192/axp192.h"

#include <Tactility/Log.h>
#include <Tactility/hal/i2c/I2c.h>
#include <Tactility/CoreDefines.h>

#include <driver/i2c.h>
#include <driver/spi_master.h>

constexpr auto* TAG = "Core2";

axp192_t axpDevice;

static int32_t axpI2cRead(TT_UNUSED void* handle, uint8_t address, uint8_t reg, uint8_t* buffer, uint16_t size) {
    if (tt::hal::i2c::masterReadRegister(I2C_NUM_0, address, reg, buffer, size, 50 / portTICK_PERIOD_MS)) {
        return AXP192_OK;
    } else {
        return 1;
    }
}

static int32_t axpI2cWrite(TT_UNUSED void* handle, uint8_t address, uint8_t reg, const uint8_t* buffer, uint16_t size) {
    if (tt::hal::i2c::masterWriteRegister(I2C_NUM_0, address, reg, buffer, size, 50 / portTICK_PERIOD_MS)) {
        return AXP192_OK;
    } else {
        return 1;
    }
}

void initAxp() {
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
}

bool initBoot() {
    TT_LOG_I(TAG, "initBoot");
    initAxp();
    return true;
}