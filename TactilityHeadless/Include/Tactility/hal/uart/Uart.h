#pragma once

#include <Tactility/RtosCompat.h>

#include "../Gpio.h"
#include "Tactility/Lockable.h"
#include "UartCompat.h"

#include <memory>
#include <vector>

namespace tt::hal::uart {

constexpr TickType_t defaultTimeout = 10 / portTICK_PERIOD_MS;

enum class InitMode {
    ByTactility, // Tactility will initialize it in the correct bootup phase
    ByExternal, // The device is already initialized and Tactility should assume it works
    Disabled // Not initialized by default
};

struct Configuration {
    uart_port_t port;
    /** Whether this bus should be initialized when device starts up */
    InitMode initMode;
    /** Whether this bus can stopped and re-started. */
    bool canReinit;
    /** Whether .config can be changed. */
    bool hasMutableConfiguration;
    /** Receive GPIO pin */
    gpio_num_t rxPin;
    /** Transmit GPIO pin */
    gpio_num_t txPin;
    /** Read-To-Send GPIO pin */
    gpio_num_t rtsPin;
    /** Clear-To-Send Send GPIO pin */
    gpio_num_t ctsPin;
    /** Receive buffer size in bytes */
    unsigned int rxBufferSize;
    /** Transmit buffer size in bytes */
    unsigned int txBufferSize;
    /** Native configuration */
    uart_config_t config;
};

enum class Status {
    Started,
    Stopped,
    Unknown
};

/** Start communications */
bool start(uart_port_t port);

/** Stop communications */
bool stop(uart_port_t port);

/** @return true when communications were successfully started */
bool isStarted(uart_port_t port);

/** @return a lock that is usable for using ESP-IDF directly, or for use with third party APIs */
Lockable& getLock(uart_port_t port);

/**
 * Read up to a specified amount of bytes
 * @param[in] port
 * @param[out] buffer
 * @param[in] bufferSize
 * @param[in] timeout
 * @return the amount of bytes that were read
 */
size_t readBytes(uart_port_t port, uint8_t* buffer, size_t bufferSize, TickType_t timeout = defaultTimeout);

/** Read a single byte */
bool readByte(uart_port_t port, uint8_t* output, TickType_t timeout = defaultTimeout);

/**
 * Read up to a specified amount of bytes
 * @param[in] port
 * @param[in] buffer
 * @param[in] bufferSize
 * @param[in] timeout
 * @return the amount of bytes that were read
 */
size_t writeBytes(uart_port_t port, const uint8_t* buffer, size_t bufferSize, TickType_t timeout = defaultTimeout);

/**
 * Write a string (excluding null terminator character)
 * @param[in] port
 * @param[in] buffer
 * @param[in] timeout
 * @return the amount of bytes that were written
 */
bool writeString(uart_port_t port, const char* buffer, TickType_t timeout = defaultTimeout);

/** @return the amount of bytes available for reading */
size_t available(uart_port_t port, TickType_t timeout = defaultTimeout);

/** Set the baud rate for the specified port */
bool setBaudRate(uart_port_t port, uint32_t baudRate, TickType_t timeout = defaultTimeout);

/** Get the baud rate for the specified port */
uint32_t getBaudRate(uart_port_t port);

/** Flush input buffers */
void flushInput(uart_port_t port);

/**
 * Read a buffer as a string until the specified character (the "untilChar" is included in the result)
 * @warning if the input data doesn't return "untilByte" then the device goes out of memory
 */
std::string readStringUntil(uart_port_t port, char untilChar, TickType_t timeout = defaultTimeout);

/** Read a buffer as a byte array until the specified character (the "untilChar" is included in the result) */
bool readUntil(uart_port_t port, uint8_t* buffer, size_t bufferSize, uint8_t untilByte, TickType_t timeout = defaultTimeout);

} // namespace tt::hal::uart
