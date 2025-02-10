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

static bool configureLocked(uart_port_t port, const uart_config_t& configuration) {
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

bool configure(uart_port_t port, const uart_config_t& configuration) {
    if (lock(port)) {
        bool result = configureLocked(port, configuration);
        unlock(port);
        return result;
    } else {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }
}

static bool startLocked(uart_port_t port) {
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

    result = uart_set_baudrate(config.port, config.baudRate);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "(%d) Starting: Failed to set baud rate to %d: %s", port, config.baudRate, esp_err_to_name(result));
    }

    result = uart_driver_install(config.port, (int)config.rxBufferSize, (int)config.txBufferSize, 0, nullptr, intr_alloc_flags);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "(%d) Starting: Failed to install driver: %s", port, esp_err_to_name(result));
        return false;
    }

    result = uart_set_baudrate(config.port, config.baudRate);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "(%d) Starting: Failed to set baud rate to %d: %s", port, config.baudRate, esp_err_to_name(result));
    }

#endif // ESP_PLATFORM

    data.isStarted = true;

    TT_LOG_I(TAG, "(%d) Started", port);
    return true;
}

bool start(uart_port_t port) {
    if (lock(port)) {
        bool result = startLocked(port);
        unlock(port);
        return result;
    } else {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }
}

static bool stopLocked(uart_port_t port) {
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

    data.isStarted = true;

    TT_LOG_I(TAG, "(%d) Stopped", port);
    return true;
}

bool stop(uart_port_t port) {
    if (lock(port)) {
        bool result = stopLocked(port);
        unlock(port);
        return result;
    } else {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }
}

bool isStarted(uart_port_t port) {
    if (lock(port, 50 / portTICK_PERIOD_MS)) {
        bool started = dataArray[port].isStarted;
        unlock(port);
        return started;
    } else {
        // If we can't get a lock, we assume the device is busy and thus has started
        return true;
    }
}

bool lock(uart_port_t port, TickType_t timeout) {
    return dataArray[port].mutex.lock(timeout);
}

bool unlock(uart_port_t port) {
    return dataArray[port].mutex.unlock();
}

size_t read(uart_port_t port, uint8_t* buffer, size_t bufferSize, TickType_t timeout) {
#ifdef ESP_PLATFORM
    auto start_time = kernel::getTicks();
    if (lock(port, timeout)) {
        auto lock_time = kernel::getTicks() - start_time;
        auto remaining_timeout = std::max(timeout - lock_time, 0UL);
        auto result = uart_read_bytes(port, buffer, bufferSize, remaining_timeout);
        unlock(port);
        return result;
    } else {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "read()");
    }
#endif // ESP_PLATFORM
    return 0;
}

bool readByte(uart_port_t port, uint8_t* output, TickType_t timeout) {
    return read(port, output, 1, timeout) == 1;
}

size_t write(uart_port_t port, const uint8_t* buffer, size_t bufferSize, TickType_t timeout) {
#ifdef ESP_PLATFORM
    if (lock(port, timeout)) {
        auto result = uart_write_bytes(port, buffer, bufferSize);
        unlock(port);
        return result;
    } else {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "write()");
    }
#endif // ESP_PLATFORM
    return 0;
}

bool writeString(uart_port_t port, const char* buffer, TickType_t timeout) {
    while (*buffer != 0) {
        if (write(port, (const uint8_t*)buffer, 1, timeout)) {
            buffer++;
        } else {
            TT_LOG_E(TAG, "Failed to write - breaking off");
            return false;
        }
    }

    return true;
}

size_t available(uart_port_t port, TickType_t timeout) {
#ifdef ESP_PLATFORM
    size_t size = 0;
    if (lock(port, timeout)) {
        uart_get_buffered_data_len(port, &size);
        unlock(port);
        return size;
    } else {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "write()");
    }
#endif // ESP_PLATFORM
    return 0;
}

void flush(uart_port_t port, TickType_t timeout) {
#ifdef ESP_PLATFORM
    size_t size = 0;
    if (lock(port, timeout)) {
        uart_flush(port);
        unlock(port);
    } else {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "write()");
    }
#endif // ESP_PLATFORM
}

void flushInput(uart_port_t port, TickType_t timeout) {
#ifdef ESP_PLATFORM
    size_t size = 0;
    if (lock(port, timeout)) {
        uart_flush_input(port);
        unlock(port);
    } else {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "write()");
    }
#endif // ESP_PLATFORM
}


bool setBaudRate(uart_port_t port, int baudRate, TickType_t timeout) {
#ifdef ESP_PLATFORM
    size_t size = 0;
    if (lock(port, timeout)) {
        uart_set_baudrate(port, baudRate);
        unlock(port);
    } else {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "write()");
    }
#endif // ESP_PLATFORM
    return true;
}

bool readUntil(uart_port_t port, uint8_t* buffer, size_t bufferSize, uint8_t untilByte, TickType_t timeout) {
    bool success = false;
    size_t index = 0;
    size_t index_limit = bufferSize - 1;
    while (readByte(port, buffer, timeout) && index < index_limit) {
        if (*buffer == untilByte) {
            success = true;
            // We have the extra space because index < index_limit
            if (buffer++) {
                *buffer = 0;
            }
            break;
        }
        buffer++;
    }
    return success;
}

std::string readStringUntil(uart_port_t port, char untilChar, TickType_t timeout) {
    std::stringstream output;
    char buffer;
    bool success = false;
    while (readByte(port, (uint8_t*)&buffer, timeout)) {
        if (buffer == untilChar) {
            success = true;
            break;
        }
        output << buffer;
    }

    if (success) {
        return output.str();
    } else {
        return {};
    }
}

} // namespace tt::hal::uart
