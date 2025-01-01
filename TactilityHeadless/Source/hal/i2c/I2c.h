#pragma once

#include "I2cCompat.h"
#include "CoreTypes.h"
#include "RtosCompat.h"
#include <climits>
#include <string>
#include <vector>

namespace tt::hal::i2c {

typedef enum {
    InitByTactility, // Tactility will initialize it in the correct bootup phase
    InitByExternal, // The device is already initialized and Tactility should assume it works
    InitDisabled // Not initialized by default
} InitMode;

typedef struct {
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
} Configuration;

bool init(const std::vector<i2c::Configuration>& configurations);

bool start(i2c_port_t port);
bool stop(i2c_port_t port);
bool isStarted(i2c_port_t port);
bool masterRead(i2c_port_t port, uint8_t address, uint8_t* data, size_t dataSize, TickType_t timeout);
esp_err_t masterRead(i2c_port_t port, uint8_t address, uint8_t reg, uint8_t* data, size_t dataSize, TickType_t timeout);
bool masterWrite(i2c_port_t port, uint16_t address, const uint8_t* data, uint16_t dataSize, TickType_t timeout);
esp_err_t masterWrite(i2c_port_t port, uint16_t address, uint8_t reg, const uint8_t* data, uint16_t dataSize, TickType_t timeout);
bool masterWriteRead(i2c_port_t port, uint8_t address, const uint8_t* writeData, size_t writeDataSize, uint8_t* readData, size_t readDataSize, TickType_t timeout);
bool masterCheckAddressForDevice(i2c_port_t port, uint8_t address, TickType_t timeout);
TtStatus lock(i2c_port_t port, TickType_t timeout = UINT_MAX);
TtStatus unlock(i2c_port_t port);

} // namespace
