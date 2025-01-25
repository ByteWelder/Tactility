#pragma once

#include "I2cCompat.h"
#include "RtosCompat.h"
#include <climits>
#include <string>
#include <vector>

namespace tt::hal::i2c {

enum class InitMode {
    ByTactility, // Tactility will initialize it in the correct bootup phase
    ByExternal, // The device is already initialized and Tactility should assume it works
    Disabled // Not initialized by default
};

struct Configuration {
    std::string name;
    /** The port to operate on */
    i2c_port_t port;
    /** Whether this bus should be initialized when device starts up */
    InitMode initMode;
    /** Whether this bus can stopped and re-started. */
    bool canReinit;
    /** Whether configuration can be changed. */
    bool hasMutableConfiguration;
    /** Configuration that must be valid when initAtBoot is set to true. */
    i2c_config_t config;
};

enum class Status {
    Started,
    Stopped,
    Unknown
};

bool init(const std::vector<i2c::Configuration>& configurations);

bool start(i2c_port_t port);
bool stop(i2c_port_t port);
bool isStarted(i2c_port_t port);

bool masterRead(i2c_port_t port, uint8_t address, uint8_t* data, size_t dataSize, TickType_t timeout);
bool masterReadRegister(i2c_port_t port, uint8_t address, uint8_t reg, uint8_t* data, size_t dataSize, TickType_t timeout);
bool masterWrite(i2c_port_t port, uint8_t address, const uint8_t* data, uint16_t dataSize, TickType_t timeout);
bool masterWriteRegister(i2c_port_t port, uint8_t address, uint8_t reg, const uint8_t* data, uint16_t dataSize, TickType_t timeout);
bool masterWriteRegisterArray(i2c_port_t port, uint8_t address, const uint8_t* data, uint16_t dataSize, TickType_t timeout);
bool masterWriteRead(i2c_port_t port, uint8_t address, const uint8_t* writeData, size_t writeDataSize, uint8_t* readData, size_t readDataSize, TickType_t timeout);
bool masterHasDeviceAtAddress(i2c_port_t port, uint8_t address, TickType_t timeout);

bool lock(i2c_port_t port, TickType_t timeout = 10 / portTICK_PERIOD_MS);
bool unlock(i2c_port_t port);

} // namespace
