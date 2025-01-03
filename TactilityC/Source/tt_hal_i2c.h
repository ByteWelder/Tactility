#pragma once

#include <freertos/FreeRTOS.h>
#include <hal/i2c_types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool tt_hal_i2c_start(i2c_port_t port);
bool tt_hal_i2c_stop(i2c_port_t port);
bool tt_hal_i2c_is_started(i2c_port_t port);

bool tt_hal_i2c_master_read(i2c_port_t port, uint8_t address, uint8_t* data, size_t dataSize, TickType_t timeout);
bool tt_hal_i2c_master_read_register(i2c_port_t port, uint8_t address, uint8_t reg, uint8_t* data, size_t dataSize, TickType_t timeout);
bool tt_hal_i2c_master_write(i2c_port_t port, uint16_t address, const uint8_t* data, uint16_t dataSize, TickType_t timeout);
bool tt_hal_i2c_master_write_register(i2c_port_t port, uint16_t address, uint8_t reg, const uint8_t* data, uint16_t dataSize, TickType_t timeout);
bool tt_hal_i2c_master_write_read(i2c_port_t port, uint8_t address, const uint8_t* writeData, size_t writeDataSize, uint8_t* readData, size_t readDataSize, TickType_t timeout);
bool tt_hal_i2c_master_has_device_at_address(i2c_port_t port, uint8_t address, TickType_t timeout);

bool tt_hal_i2c_lock(i2c_port_t port, TickType_t timeout);
bool tt_hal_i2c_unlock(i2c_port_t port);

#ifdef __cplusplus
}
#endif