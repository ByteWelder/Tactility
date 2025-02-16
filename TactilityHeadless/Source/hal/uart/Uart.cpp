#include "Tactility/hal/uart/Uart.h"

#include <Tactility/Log.h>
#include <Tactility/LogMessages.h>
#include <Tactility/Mutex.h>

#include <sstream>

#ifdef ESP_PLATFORM
#include <esp_check.h>
#endif

#define TAG "uart"

namespace tt::hal::uart {

struct Data {
    Mutex mutex;
    bool isConfigured = false;
    bool isStarted = false;
    Configuration configuration;
};

static Data dataArray[UART_NUM_MAX];

static const char* initModeToString(InitMode mode) {
    switch (mode) {
        using enum InitMode;
        case ByTactility:
            return TT_STRINGIFY(InitMode::ByTactility);
        case ByExternal:
            return TT_STRINGIFY(InitMode::ByExternal);
        case Disabled:
            return TT_STRINGIFY(InitMode::Disabled);
    }
    tt_crash("not implemented");
}

static void printInfo(const Data& data) {
    TT_LOG_D(TAG, "UART info for port %d", data.configuration.port);
    TT_LOG_D(TAG, "  isStarted: %d", data.isStarted);
    TT_LOG_D(TAG, "  isConfigured: %d", data.isConfigured);
    TT_LOG_D(TAG, "  initMode: %s", initModeToString(data.configuration.initMode));
    TT_LOG_D(TAG, "  canReinit: %d", data.configuration.canReinit);
    TT_LOG_D(TAG, "  hasMutableConfiguration: %d", data.configuration.hasMutableConfiguration);
    TT_LOG_D(TAG, "  RX pin: %d", data.configuration.rxPin);
    TT_LOG_D(TAG, "  TX pin: %d", data.configuration.txPin);
    TT_LOG_D(TAG, "  RTS pin: %d", data.configuration.rtsPin);
    TT_LOG_D(TAG, "  CTS pin: %d", data.configuration.ctsPin);
}

bool init(const std::vector<uart::Configuration>& configurations) {
    TT_LOG_I(TAG, "Init");
    for (const auto& configuration: configurations) {
        Data& data = dataArray[configuration.port];
        data.configuration = configuration;
        data.isConfigured = true;
    }

    for (const auto& config: configurations) {
        printInfo(dataArray[config.port]);
        if (config.initMode == InitMode::ByTactility) {
            if (!start(config.port)) {
                return false;
            }
        } else if (config.initMode == InitMode::ByExternal) {
            dataArray[config.port].isStarted = true;
        }
    }

    return true;
}

bool configure(uart_port_t port, const uart_config_t& configuration) {
    auto lock = getLock(port).asScopedLock();
    lock.lock();

    Data& data = dataArray[port];
    if (data.isStarted) {
        TT_LOG_E(TAG, "(%d) Cannot reconfigure while interface is started", port);
        return false;
    } else if (!data.configuration.hasMutableConfiguration) {
        TT_LOG_E(TAG, "(%d) Mutation not allowed by original configuration", port);
        return false;
    } else {
        data.configuration.config = configuration;
        return true;
    }
}

bool start(uart_port_t port) {
    auto lock = getLock(port).asScopedLock();
    lock.lock();

    Data& data = dataArray[port];
    printInfo(data);

    if (data.isStarted) {
        TT_LOG_E(TAG, "(%d) Starting: Already started", port);
        return false;
    }

    if (!data.isConfigured) {
        TT_LOG_E(TAG, "(%d) Starting: Not configured", port);
        return false;
    }

#ifdef ESP_PLATFORM

    Configuration& config = data.configuration;

    int intr_alloc_flags;
#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#else
    intr_alloc_flags = 0;
#endif

    esp_err_t result = uart_param_config(config.port, &config.config);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "(%d) Starting: Failed to configure: %s", port, esp_err_to_name(result));
        return false;
    }

    result = uart_set_pin(config.port, config.txPin, config.rxPin, config.rtsPin, config.ctsPin);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "(%d) Starting: Failed set pins: %s", port, esp_err_to_name(result));
        return false;
    }

    result = uart_driver_install(config.port, (int)config.rxBufferSize, (int)config.txBufferSize, 0, nullptr, intr_alloc_flags);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "(%d) Starting: Failed to install driver: %s", port, esp_err_to_name(result));
        return false;
    }

#endif // ESP_PLATFORM

    data.isStarted = true;

    TT_LOG_I(TAG, "(%d) Started", port);
    return true;
}

bool stop(uart_port_t port) {
    auto lock = getLock(port).asScopedLock();
    lock.lock();

    Data& data = dataArray[port];
    Configuration& config = data.configuration;

    if (!config.canReinit) {
        TT_LOG_E(TAG, "(%d) Stopping: Not allowed to re-init", port);
        return false;
    }

    if (!data.isStarted) {
        TT_LOG_E(TAG, "(%d) Stopping: Not started", port);
        return false;
    }

#ifdef ESP_PLATFORM
    esp_err_t result = uart_driver_delete(port);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "(%d) Stopping: Failed to delete driver: %s", port, esp_err_to_name(result));
        return false;
    }
#endif // ESP_PLATFORM

    data.isStarted = false;

    TT_LOG_I(TAG, "(%d) Stopped", port);
    return true;
}

bool isStarted(uart_port_t port) {
    auto lock = getLock(port).asScopedLock();
    lock.lock();

    return dataArray[port].isStarted;
}

Lock& getLock(uart_port_t port) {
    return dataArray[port].mutex;
}

size_t readBytes(uart_port_t port, uint8_t* buffer, size_t bufferSize, TickType_t timeout) {
    auto lock = getLock(port).asScopedLock();
    if (!lock.lock(timeout)) {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }

#ifdef ESP_PLATFORM
    auto start_time = kernel::getTicks();
    auto lock_time = kernel::getTicks() - start_time;
    auto remaining_timeout = std::max(timeout - lock_time, 0UL);
    auto result = uart_read_bytes(port, buffer, bufferSize, remaining_timeout);
    return result;
#endif // ESP_PLATFORM
    return 0;
}

bool readByte(uart_port_t port, uint8_t* output, TickType_t timeout) {
    return readBytes(port, output, 1, timeout) == 1;
}

size_t writeBytes(uart_port_t port, const uint8_t* buffer, size_t bufferSize, TickType_t timeout) {
    auto lock = getLock(port).asScopedLock();
    if (!lock.lock(timeout)) {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }

#ifdef ESP_PLATFORM
    return uart_write_bytes(port, buffer, bufferSize);
#endif // ESP_PLATFORM
    return 0;
}

bool writeString(uart_port_t port, const char* buffer, TickType_t timeout) {
    while (*buffer != 0) {
        if (writeBytes(port, (const uint8_t*)buffer, 1, timeout)) {
            buffer++;
        } else {
            TT_LOG_E(TAG, "Failed to write - breaking off");
            return false;
        }
    }

    return true;
}

size_t available(uart_port_t port, TickType_t timeout) {
    auto lock = getLock(port).asScopedLock();
    if (!lock.lock(timeout)) {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }

#ifdef ESP_PLATFORM
    size_t size = 0;
    uart_get_buffered_data_len(port, &size);
    return size;
#else
    return 0;
#endif // ESP_PLATFORM
}

void flush(uart_port_t port) {
#ifdef ESP_PLATFORM
    uart_flush(port);
#endif // ESP_PLATFORM
}

void flushInput(uart_port_t port) {
#ifdef ESP_PLATFORM
    uart_flush_input(port);
#endif // ESP_PLATFORM
}

uint32_t getBaudRate(uart_port_t port) {
#ifdef ESP_PLATFORM
    uint32_t baud_rate = 0;
    auto result = uart_get_baudrate(port, &baud_rate);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    return baud_rate;
#else
    return 0;
#endif
}

bool setBaudRate(uart_port_t port, uint32_t baudRate, TickType_t timeout) {
    auto lock = getLock(port).asScopedLock();
    if (!lock.lock(timeout)) {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }

#ifdef ESP_PLATFORM
    auto result = uart_set_baudrate(port, baudRate);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    return result == ESP_OK;
#else
    return true;
#endif // ESP_PLATFORM
}

// #define DEBUG_READ_UNTIL

size_t readUntil(uart_port_t port, uint8_t* buffer, size_t bufferSize, uint8_t untilByte, TickType_t timeout, bool addNullTerminator) {
    TickType_t start_time = kernel::getTicks();
    uint8_t* buffer_write_ptr = buffer;
    uint8_t* buffer_limit = buffer + bufferSize - 1; // Keep 1 extra char as mull terminator
    TickType_t timeout_left = timeout;
    while (readByte(port, buffer_write_ptr, timeout_left) && buffer_write_ptr < buffer_limit) {
#ifdef DEBUG_READ_UNTIL
        // If first successful read and we're not receiving an empty response
        if (buffer_write_ptr == buffer && *buffer_write_ptr != 0x00U && *buffer_write_ptr != untilByte) {
            printf(">>");
        }
#endif

        if (*buffer_write_ptr == untilByte) {
            // TODO: Fix when untilByte is null terminator char already
            if (addNullTerminator) {
                buffer_write_ptr++;
                *buffer_write_ptr = 0x00U;
            }
            break;
//        } else if (*buffer_write_ptr == 0x00U) {
//            break;
        }

#ifdef DEBUG_READ_UNTIL
        printf("%c", *buffer_write_ptr);
#endif

        buffer_write_ptr++;

        TickType_t now = kernel::getTicks();
        if (now > (start_time + timeout)) {
#ifdef DEBUG_READ_UNTIL
            TT_LOG_W(TAG, "readUntil() timeout");
#endif
            break;
        } else {
            timeout_left = timeout - (now - start_time);
        }
    }

#ifdef DEBUG_READ_UNTIL
    // If we read data and it's not an empty response
    if (buffer_write_ptr != buffer && *buffer != 0x00U && *buffer != untilByte) {
        printf("\n");
    }
#endif

    if (addNullTerminator && (buffer_write_ptr > buffer)) {
        return (uint32_t)buffer_write_ptr - (uint32_t)buffer - 1UL;
    } else {
        return (uint32_t)buffer_write_ptr - (uint32_t)buffer;
    }
}

} // namespace tt::hal::uart
