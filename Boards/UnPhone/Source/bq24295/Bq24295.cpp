#include "Bq24295.h"
#include "Log.h"

#define TAG "bq24295"

/** Reference: https://gitlab.com/hamishcunningham/unphonelibrary/-/blob/main/unPhone.h?ref_type=heads */
namespace registers {
    static const uint8_t WATCHDOG = 0x05U; // Charge end/timer cntrl
    static const uint8_t OPERATION_CONTROL = 0x07U; // Misc operation control
    static const uint8_t STATUS = 0x08U; // System status
    static const uint8_t VERSION = 0x0AU; // Vendor/part/revision status
} // namespace registers

// region Watchdog
bool Bq24295::getWatchDog(uint8_t value) const {
    return readRegister8(registers::WATCHDOG, value);
}

bool Bq24295::setWatchDogBitOn(uint8_t mask) const {
    return bitOn(registers::WATCHDOG, mask);
}

bool Bq24295::setWatchDogBitOff(uint8_t mask) const {
    return bitOff(registers::WATCHDOG, mask);
}

// endregoin

// region Operation Control

bool Bq24295::getOperationControl(uint8_t value) const {
    return readRegister8(registers::OPERATION_CONTROL, value);
}

bool Bq24295::setOperationControlBitOn(uint8_t mask) const {
    return bitOn(registers::OPERATION_CONTROL, mask);
}

bool Bq24295::setOperationControlBitOff(uint8_t mask) const {
    return bitOff(registers::OPERATION_CONTROL, mask);
}

// endregion

// region Other

bool Bq24295::getStatus(uint8_t& value) const {
    return readRegister8(registers::STATUS, value);
}

bool Bq24295::getVersion(uint8_t& value) const {
    return readRegister8(registers::VERSION, value);
}

void Bq24295::printInfo() const {
    uint8_t version, status;
    if (getStatus(status) && getVersion(version)) {
        TT_LOG_I(TAG, "Version %d, status %02x", version, status);
    } else {
        TT_LOG_E(TAG, "Failed to retrieve version and/or status");
    }
}

// endregion