#pragma once

#include <Tactility/RtosCompat.h>

#include "../Gpio.h"
#include "Tactility/Lock.h"
#include "UartCompat.h"
#include "Tactility/hal/uart/Configuration.h"

#include <memory>
#include <vector>

namespace tt::hal::uart {

constexpr TickType_t defaultTimeout = 10 / portTICK_PERIOD_MS;

enum class InitMode {
    ByTactility, // Tactility will initialize it in the correct bootup phase
    ByExternal, // The device is already initialized and Tactility should assume it works
    Disabled // Not initialized by default
};

enum class Status {
    Started,
    Stopped,
    Unknown
};

class Uart {

private:

    uint32_t id;

public:

    Uart();
    virtual ~Uart();

    inline uint32_t getId() const { return id; }

    virtual bool start() = 0;
    virtual bool isStarted() const = 0;

    virtual bool stop() = 0;

    /**
     * Read up to a specified amount of bytes
     * @param[out] buffer
     * @param[in] bufferSize
     * @param[in] timeout
     * @return the amount of bytes that were read
     */
    virtual size_t readBytes(std::byte* buffer, size_t bufferSize, TickType_t timeout = defaultTimeout) = 0;

    size_t readBytes(std::uint8_t* buffer, size_t bufferSize, TickType_t timeout = defaultTimeout) {
        return readBytes(reinterpret_cast<std::byte*>(buffer), bufferSize, timeout);
    }

    /** Read a single byte */
    virtual bool readByte(std::byte* output, TickType_t timeout = defaultTimeout) = 0;

    inline bool readByte(char* output, TickType_t timeout = defaultTimeout) {
        return readByte(reinterpret_cast<std::byte*>(output), timeout);
    }

    inline bool readByte(uint8_t* output, TickType_t timeout = defaultTimeout) {
        return readByte(reinterpret_cast<std::byte*>(output), timeout);
    }

    /**
     * Read up to a specified amount of bytes
     * @param[in] buffer
     * @param[in] bufferSize
     * @param[in] timeout
     * @return the amount of bytes that were read
     */
    virtual size_t writeBytes(const std::byte* buffer, size_t bufferSize, TickType_t timeout = defaultTimeout) = 0;

    inline size_t writeBytes(const std::uint8_t* buffer, size_t bufferSize, TickType_t timeout = defaultTimeout) {
        return writeBytes(reinterpret_cast<const std::byte*>(buffer), bufferSize, timeout);
    }

    /** @return the amount of bytes available for reading */
    virtual size_t available(TickType_t timeout = defaultTimeout) = 0;

    /** Set the baud rate for the specified port */
    virtual bool setBaudRate(uint32_t baudRate, TickType_t timeout = defaultTimeout) = 0;

    /** Get the baud rate for the specified port */
    virtual uint32_t getBaudRate() = 0;

    /** Flush input buffers */
    virtual void flushInput() = 0;

    /**
     * Write a string (excluding null terminator character)
     * @param[in] buffer
     * @param[in] timeout
     * @return the amount of bytes that were written
     */
    bool writeString(const char* buffer, TickType_t timeout = defaultTimeout);

    /**
     * Read a buffer as a string until the specified character (the "untilChar" is included in the result)
     * @warning if the input data doesn't return "untilByte" then the device goes out of memory
     */
    std::string readStringUntil(char untilChar, TickType_t timeout = defaultTimeout);

    /**
     * Read a buffer as a byte array until the specified character (the "untilChar" is included in the result)
     * @return the amount of bytes read from UART
     */
    size_t readUntil(std::byte* buffer, size_t bufferSize, uint8_t untilByte, TickType_t timeout = defaultTimeout, bool addNullTerminator = true);
};

/**
 * Opens a UART.
 * @param[in] name the UART name as specified in the board configuration.
 * @return the UART when it was successfully opened, or nullptr when it is in use.
 */
std::unique_ptr<Uart> open(std::string name);

std::vector<std::string> getNames();

} // namespace tt::hal::uart
