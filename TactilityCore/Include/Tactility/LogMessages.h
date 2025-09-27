/**
 * Contains common log messages.
 * This helps to keep the binary smaller.
 */
#pragma once

// Crashes
#define LOG_MESSAGE_ILLEGAL_STATE "Illegal state"

// Alloc
#define LOG_MESSAGE_ALLOC_FAILED "Out of memory"
#define LOG_MESSAGE_ALLOC_FAILED_FMT "Out of memory (failed to allocated %zu bytes)"

// Mutex
#define LOG_MESSAGE_MUTEX_LOCK_FAILED "Mutex acquisition timeout"
#define LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT "Mutex acquisition timeout (%s)"

// SPI
#define LOG_MESSAGE_SPI_INIT_START_FMT "SPI %d init"
#define LOG_MESSAGE_SPI_INIT_FAILED_FMT "SPI %d init failed"

// I2C
#define LOG_MESSAGE_I2C_INIT_START "I2C init"
#define LOG_MESSAGE_I2C_INIT_CONFIG_FAILED "I2C config failed"
#define LOG_MESSAGE_I2C_INIT_DRIVER_INSTALL_FAILED "I2C driver install failed"

// Power on
#define LOG_MESSAGE_POWER_ON_START "Power on"
#define LOG_MESSAGE_POWER_ON_FAILED "Power on failed"
