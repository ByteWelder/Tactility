#pragma once

#include "./I2cCompat.h"
#include "Tactility/Lock.h"

#include <Tactility/RtosCompat.h>

#include <climits>
#include <string>

namespace tt::hal::i2c {

constexpr TickType_t defaultTimeout = 10 / portTICK_PERIOD_MS;

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
    /**
     * Whether this bus can be changed after booting.
     * If the bus is internal and/or used for core features like touch screen, then it can be declared static.
     */
    bool isMutable;
    /** Configuration that must be valid when initAtBoot is set to true. */
    i2c_config_t config;
};

enum class Status {
    Started,
    Stopped,
    Unknown
};

/**
 * Reconfigure a port with the provided settings.
 * @warning This fails when the HAL Configuration is not mutable.
 * @param[in] port the port to reconfigure
 * @param[in] configuration the new configuration
 * @return true on success
 */
bool configure(i2c_port_t port, const i2c_config_t& configuration);

/**
 * Start the bus for the specified port.
 * Devices might be started automatically at boot if their HAL configuration requires it.
 */
bool start(i2c_port_t port);

/** Stop the bus for the specified port. */
bool stop(i2c_port_t port);

/** @return true if the bus is started */
bool isStarted(i2c_port_t port);

/** Read bytes in master mode. */
bool masterRead(i2c_port_t port, uint8_t address, uint8_t* data, size_t dataSize, TickType_t timeout = defaultTimeout);

/** Read bytes from the specified register in master mode. */
bool masterReadRegister(i2c_port_t port, uint8_t address, uint8_t reg, uint8_t* data, size_t dataSize, TickType_t timeout = defaultTimeout);

/** Write bytes in master mode. */
bool masterWrite(i2c_port_t port, uint8_t address, const uint8_t* data, uint16_t dataSize, TickType_t timeout = defaultTimeout);

/** Write bytes to a register in master mode */
bool masterWriteRegister(i2c_port_t port, uint8_t address, uint8_t reg, const uint8_t* data, uint16_t dataSize, TickType_t timeout = defaultTimeout);

/**
 * Write multiple values to multiple registers in master mode.
 * The input is as follows: { register1, value1, register2, value2, ... }
 * @return false if any of the write operations failed
 */
bool masterWriteRegisterArray(i2c_port_t port, uint8_t address, const uint8_t* data, uint16_t dataSize, TickType_t timeout = defaultTimeout);

/** Write bytes and then read the response bytes in master mode*/
bool masterWriteRead(i2c_port_t port, uint8_t address, const uint8_t* writeData, size_t writeDataSize, uint8_t* readData, size_t readDataSize, TickType_t timeout = defaultTimeout);

/** @return true when a device is detected at the specified address */
bool masterHasDeviceAtAddress(i2c_port_t port, uint8_t address, TickType_t timeout = defaultTimeout);

/**
 * The lock for the specified bus.
 * This can be used when calling native I2C functionality outside of Tactility.
 */
Lock& getLock(i2c_port_t port);

} // namespace
