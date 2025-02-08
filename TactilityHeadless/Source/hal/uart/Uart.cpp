#include "Tactility/hal/uart/Uart.h"

#include <Tactility/Log.h>
#include <Tactility/LogMessages.h>
#include <Tactility/Mutex.h>

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
    TT_LOG_V(TAG, "UART info for port %d", data.configuration.port);
    TT_LOG_V(TAG, "  isStarted: %d", data.isStarted);
    TT_LOG_V(TAG, "  isConfigured: %d", data.isConfigured);
    TT_LOG_V(TAG, "  initMode: %s", initModeToString(data.configuration.initMode));
    TT_LOG_V(TAG, "  canReinit: %d", data.configuration.canReinit);
    TT_LOG_V(TAG, "  hasMutableConfiguration: %d", data.configuration.hasMutableConfiguration);
    TT_LOG_V(TAG, "  RX pin: %d", data.configuration.rxPin);
    TT_LOG_V(TAG, "  TX pin: %d", data.configuration.txPin);
    TT_LOG_V(TAG, "  RTS pin: %d", data.configuration.rtsPin);
    TT_LOG_V(TAG, "  CTS pin: %d", data.configuration.ctsPin);
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
    } else {
        data.isStarted = false;
    }
#else
    data.isStarted = true;
#endif // ESP_PLATFORM

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

} // namespace tt::hal::uart
