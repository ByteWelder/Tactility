#include "Axp2101.h"
#include "Log.h"

bool Axp2101::readRegister12(uint8_t reg, float& out) const {
    std::uint8_t data[2] = {0};
    if (tt::hal::i2c::masterReadRegister(port, DEFAULT_ADDRESS, reg, data, 2, DEFAULT_TIMEOUT) == ESP_OK) {
        out = (data[0] & 0x0F) << 8 | data[1];
        return true;
    } else {
        return false;
    }
}

bool Axp2101::readRegister14(uint8_t reg, float& out) const {
    std::uint8_t data[2] = {0};
    if (tt::hal::i2c::masterReadRegister(port, DEFAULT_ADDRESS, reg, data, 2, DEFAULT_TIMEOUT) == ESP_OK) {
        out = (data[0] & 0x3F) << 8 | data[1];
        return true;
    } else {
        return false;
    }
}

bool Axp2101::readRegister16(uint8_t reg, uint16_t& out) const {
    std::uint8_t data[2] = {0};
    if (tt::hal::i2c::masterReadRegister(port, DEFAULT_ADDRESS, reg, data, 2, DEFAULT_TIMEOUT) == ESP_OK) {
        out = data[0] << 8 | data[1];
        return true;
    } else {
        return false;
    }
}

bool Axp2101::readRegister8(uint8_t reg, uint8_t& result) const {
    return tt::hal::i2c::masterWriteRead(port, DEFAULT_ADDRESS, &reg, 1, &result, 1, DEFAULT_TIMEOUT);
}

bool Axp2101::writeRegister8(uint8_t reg, uint8_t value) const {
    return tt::hal::i2c::masterWriteRegister(port, DEFAULT_ADDRESS, reg, &value, 1, DEFAULT_TIMEOUT);
}

bool Axp2101::getBatteryVoltage(float& vbatMillis) const {
    return readRegister14(0x34, vbatMillis);
}

bool Axp2101::getChargeStatus(ChargeStatus& status) const {
    uint8_t value;
    if (readRegister8(0x01, value)) {
        value = (value >> 5) & 0b11;
        status = (value == 1) ? CHARGE_STATUS_CHARGING : ((value == 2) ? CHARGE_STATUS_DISCHARGING : CHARGE_STATUS_STANDBY);
        return true;
    } else {
        return false;
    }
}

bool Axp2101::isChargingEnabled(bool& enabled) const {
    uint8_t value;
    if (readRegister8(0x18, value)) {
        enabled = value & 0b10;
        return true;
    } else {
        return false;
    }
}

bool Axp2101::setChargingEnabled(bool enabled) const {
    uint8_t value;
    if (readRegister8(0x18, value)) {
        return writeRegister8(0x18, (value & 0xFD) | (enabled << 1));
    } else {
        return false;
    }
}
