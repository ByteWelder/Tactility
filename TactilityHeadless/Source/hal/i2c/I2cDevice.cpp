#include "Tactility/hal/i2c/I2cDevice.h"

#include <cstdint>

namespace tt::hal::i2c {

bool I2cDevice::readRegister12(uint8_t reg, float& out) const {
    std::uint8_t data[2] = {0};
    if (tt::hal::i2c::masterReadRegister(port, address, reg, data, 2, DEFAULT_TIMEOUT)) {
        out = (data[0] & 0x0F) << 8 | data[1];
        return true;
    } else {
        return false;
    }
}

bool I2cDevice::readRegister14(uint8_t reg, float& out) const {
    std::uint8_t data[2] = {0};
    if (tt::hal::i2c::masterReadRegister(port, address, reg, data, 2, DEFAULT_TIMEOUT)) {
        out = (data[0] & 0x3F) << 8 | data[1];
        return true;
    } else {
        return false;
    }
}

bool I2cDevice::readRegister16(uint8_t reg, uint16_t& out) const {
    std::uint8_t data[2] = {0};
    if (tt::hal::i2c::masterReadRegister(port, address, reg, data, 2, DEFAULT_TIMEOUT)) {
        out = data[0] << 8 | data[1];
        return true;
    } else {
        return false;
    }
}

bool I2cDevice::readRegister8(uint8_t reg, uint8_t& result) const {
    return tt::hal::i2c::masterWriteRead(port, address, &reg, 1, &result, 1, DEFAULT_TIMEOUT);
}

bool I2cDevice::writeRegister8(uint8_t reg, uint8_t value) const {
    return tt::hal::i2c::masterWriteRegister(port, address, reg, &value, 1, DEFAULT_TIMEOUT);
}

bool I2cDevice::bitOn(uint8_t reg, uint8_t bitmask) const {
    uint8_t state;
    if (readRegister8(reg, state)) {
        state |= bitmask;
        return writeRegister8(reg, state);
    } else {
        return false;
    }
}

bool I2cDevice::bitOff(uint8_t reg, uint8_t bitmask) const {
    uint8_t state;
    if (readRegister8(reg, state)) {
        state &= ~bitmask;
        return writeRegister8(reg, state);
    } else {
        return false;
    }
}

} // namespace
