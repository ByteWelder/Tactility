#include "tt_hal_i2c.h"
#include "hal/i2c/I2c.h"

extern "C" {

bool tt_hal_i2c_start(i2c_port_t port) {
    return tt::hal::i2c::start(port);
}

bool tt_hal_i2c_stop(i2c_port_t port) {
    return tt::hal::i2c::stop(port);
}

bool tt_hal_i2c_is_started(i2c_port_t port) {
    return tt::hal::i2c::isStarted(port);
}

bool tt_hal_i2c_master_read(i2c_port_t port, uint8_t address, uint8_t* data, size_t dataSize, TickType_t timeout) {
    return tt::hal::i2c::masterRead(port, address, data, dataSize, timeout);
}

bool tt_hal_i2c_master_read_register(i2c_port_t port, uint8_t address, uint8_t reg, uint8_t* data, size_t dataSize, TickType_t timeout) {
    return tt::hal::i2c::masterRead(port, address, reg, data, dataSize, timeout);
}

bool tt_hal_i2c_master_write(i2c_port_t port, uint16_t address, const uint8_t* data, uint16_t dataSize, TickType_t timeout) {
    return tt::hal::i2c::masterWrite(port, address, data, dataSize, timeout);
}

bool tt_hal_i2c_master_write_register(i2c_port_t port, uint16_t address, uint8_t reg, const uint8_t* data, uint16_t dataSize, TickType_t timeout) {
    return tt::hal::i2c::masterWrite(port, address, reg, data, dataSize, timeout);
}

bool tt_hal_i2c_master_write_read(i2c_port_t port, uint8_t address, const uint8_t* writeData, size_t writeDataSize, uint8_t* readData, size_t readDataSize, TickType_t timeout) {
    return tt::hal::i2c::masterWriteRead(port, address, writeData, writeDataSize, readData, readDataSize, timeout);
}

bool tt_hal_i2c_master_has_device_at_address(i2c_port_t port, uint8_t address, TickType_t timeout) {
    return tt::hal::i2c::masterHasDeviceAtAddress(port, address, timeout);
}

bool tt_hal_i2c_lock(i2c_port_t port, TickType_t timeout) {
    return tt::hal::i2c::lock(port, timeout);
}

bool tt_hal_i2c_unlock(i2c_port_t port) {
    return tt::hal::i2c::unlock(port);
}

}