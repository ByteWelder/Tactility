#include "Aw9523.h"

#define AW9523_REGISTER_P0 0x02
#define AW9523_REGISTER_P1 0x03
#define AW9523_REGISTER_CTL 0x11

bool Aw9523::readP0(uint8_t& output) const {
    return readRegister8(AW9523_REGISTER_P0, output);
}

bool Aw9523::readP1(uint8_t& output) const {
    return readRegister8(AW9523_REGISTER_P1, output);
}

bool Aw9523::readCTL(uint8_t& output) const {
    return readRegister8(AW9523_REGISTER_CTL, output);
}

bool Aw9523::writeP0(uint8_t value) const {
    return writeRegister8(AW9523_REGISTER_P0, value);
}

bool Aw9523::writeP1(uint8_t value) const {
    return writeRegister8(AW9523_REGISTER_P1, value);
}

bool Aw9523::writeCTL(uint8_t value) const {
    return writeRegister8(AW9523_REGISTER_CTL, value);
}

bool Aw9523::bitOnP1(uint8_t bitmask) const {
    return bitOn(AW9523_REGISTER_P1, bitmask);
}
