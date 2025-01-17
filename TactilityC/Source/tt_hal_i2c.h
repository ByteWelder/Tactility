#pragma once

#include <freertos/FreeRTOS.h>
#include <hal/i2c_types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start I2C communications for the specified port
 * @param[in] port the I2C port to init
 * @return true on success
 */
bool tt_hal_i2c_start(i2c_port_t port);

/**
 * Stop I2C communications for the specified port
 * @param[in] port the I2C port to deinit
 * @return true on success
 */
bool tt_hal_i2c_stop(i2c_port_t port);

/**
 * Check if the port was successfully started.
 * @param[in] port the port to check
 * @return true when the port was successfully started
 */
bool tt_hal_i2c_is_started(i2c_port_t port);

/**
 * Read from an I2C port in master mode.
 * @param[in] port the I2C port to read from
 * @param[in] address
 * @param[in] data
 * @param[in] dataSize
 * @param[in] timeout
 */
bool tt_hal_i2c_master_read(i2c_port_t port, uint8_t address, uint8_t* data, size_t dataSize, TickType_t timeout);

/**
 * Read a register from an I2C port in master mode.
 * @param[in] port the I2C port to read from
 * @param[in] address
 * @param[in] reg
 * @param[in] data
 * @param[in] dataSize
 * @param[in] timeout
 */
bool tt_hal_i2c_master_read_register(i2c_port_t port, uint8_t address, uint8_t reg, uint8_t* data, size_t dataSize, TickType_t timeout);

/**
 * Write to an I2C port in master mode.
 * @param[in] port the I2C port to write to
 * @param[in] address
 * @param[in] data
 * @param[in] dataSize
 * @param[in] timeout
 */
bool tt_hal_i2c_master_write(i2c_port_t port, uint8_t address, const uint8_t* data, uint16_t dataSize, TickType_t timeout);

/**
 * Write to a register of an I2C port in master mode.
 * @param[in] port the I2C port to write to
 * @param[in] address
 * @param[in] reg
 * @param[in] data
 * @param[in] dataSize
 * @param[in] timeout
 */
bool tt_hal_i2c_master_write_register(i2c_port_t port, uint8_t address, uint8_t reg, const uint8_t* data, uint16_t dataSize, TickType_t timeout);

/**
 * Write then read from an I2C port in master mode.
 * @param[in] port the I2C port to communicate with
 * @param[in] address
 * @param[in] writeData
 * @param[in] writeDataSize
 * @param[in] readData
 * @param[in] readDataSize
 * @param[in] timeout
 */
bool tt_hal_i2c_master_write_read(i2c_port_t port, uint8_t address, const uint8_t* writeData, size_t writeDataSize, uint8_t* readData, size_t readDataSize, TickType_t timeout);

/**
 * Check if an I2C port has a device at the specified address.
 * @param[in] port the I2C port to communicate with
 * @param[in] address
 * @param[in] timeout
 */
bool tt_hal_i2c_master_has_device_at_address(i2c_port_t port, uint8_t address, TickType_t timeout);

/**
 * Used to lock an I2C port.
 * This is useful for creating thread-safe I2C calls while calling ESP-IDF directly of third party I2C APIs.
 * @param[in] port the I2C port to lock
 * @param[in] timeout
 */
bool tt_hal_i2c_lock(i2c_port_t port, TickType_t timeout);

/**
 * Used to unlock an I2C port.
 * This is useful for creating thread-safe I2C calls while calling ESP-IDF directly of third party I2C APIs.
 * @param[in] port the I2C port to unlock
 */
bool tt_hal_i2c_unlock(i2c_port_t port);

#ifdef __cplusplus
}
#endif