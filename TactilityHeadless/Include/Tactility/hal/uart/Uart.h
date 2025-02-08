#pragma once

#include <Tactility/RtosCompat.h>

#include "UartCompat.h"

#include <vector>
#include <memory>

namespace tt::hal::uart {

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

bool init(const std::vector<uart::Configuration>& configurations);

bool start(uart_port_t port);
bool stop(uart_port_t port);
bool isStarted(uart_port_t port);

bool lock(uart_port_t port, TickType_t timeout = 10 / portTICK_PERIOD_MS);
bool unlock(uart_port_t port);

size_t read(uart_port_t port, uint8_t* buffer, size_t bufferSize, TickType_t timeout = 10 / portTICK_PERIOD_MS);
size_t write(uart_port_t port, const uint8_t* buffer, size_t bufferSize, TickType_t timeout = 10 / portTICK_PERIOD_MS);

} // namespace tt::hal::uart
