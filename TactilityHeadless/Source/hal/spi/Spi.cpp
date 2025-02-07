#include "Tactility/hal/spi/Spi.h"

#include <Tactility/Mutex.h>

#define TAG "spi"

namespace tt::hal::spi {

struct Data {
    std::shared_ptr<Lockable> lock;
    bool isConfigured = false;
    bool isStarted = false;
    Configuration configuration;
};

static Data dataArray[SPI_HOST_MAX];

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
    TT_LOG_V(TAG, "SPI info for device %d", data.configuration.device);
    TT_LOG_V(TAG, "  isStarted: %d", data.isStarted);
    TT_LOG_V(TAG, "  isConfigured: %d", data.isConfigured);
    TT_LOG_V(TAG, "  initMode: %s", initModeToString(data.configuration.initMode));
    TT_LOG_V(TAG, "  canReinit: %d", data.configuration.canReinit);
    TT_LOG_V(TAG, "  hasMutableConfiguration: %d", data.configuration.hasMutableConfiguration);
    TT_LOG_V(TAG, "  MISO pin: %d", data.configuration.config.miso_io_num);
    TT_LOG_V(TAG, "  MOSI pin: %d", data.configuration.config.mosi_io_num);
    TT_LOG_V(TAG, "  SCLK pin: %d", data.configuration.config.sclk_io_num);
}

bool init(const std::vector<spi::Configuration>& configurations) {
    TT_LOG_I(TAG, "Init");
    for (const auto& configuration: configurations) {
        Data& data = dataArray[configuration.device];
        data.configuration = configuration;
        data.isConfigured = true;
        if (configuration.lock != nullptr) {
            data.lock = configuration.lock;
        } else {
            data.lock = std::make_shared<Mutex>();
        }
    }

    for (const auto& config: configurations) {
        printInfo(dataArray[config.device]);
        if (config.initMode == InitMode::ByTactility) {
            if (!start(config.device)) {
                return false;
            }
        } else if (config.initMode == InitMode::ByExternal) {
            dataArray[config.device].isStarted = true;
        }
    }

    return true;
}

static bool configureLocked(spi_host_device_t device, const spi_bus_config_t& configuration) {
    Data& data = dataArray[device];
    if (data.isStarted) {
        TT_LOG_E(TAG, "(%d) Cannot reconfigure while interface is started", device);
        return false;
    } else if (!data.configuration.hasMutableConfiguration) {
        TT_LOG_E(TAG, "(%d) Mutation not allowed by original configuration", device);
        return false;
    } else {
        data.configuration.config = configuration;
        return true;
    }
}

bool configure(spi_host_device_t device, const spi_bus_config_t& configuration) {
    if (lock(device)) {
        bool result = configureLocked(device, configuration);
        unlock(device);
        return result;
    } else {
        TT_LOG_E(TAG, "(%d) Mutex timeout", device);
        return false;
    }
}

static bool startLocked(spi_host_device_t device) {
    Data& data = dataArray[device];
    printInfo(data);

    if (data.isStarted) {
        TT_LOG_E(TAG, "(%d) Starting: Already started", device);
        return false;
    }

    if (!data.isConfigured) {
        TT_LOG_E(TAG, "(%d) Starting: Not configured", device);
        return false;
    }

    #ifdef ESP_PLATFORM

    Configuration& config = data.configuration;
    auto result = spi_bus_initialize(device, &data.configuration.config, data.configuration.dma);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "(%d) Starting: Failed to initialize: %s", device, esp_err_to_name(result));
        return false;
    } else {
        data.isStarted = true;
    }

    #else

    data.isStarted = true;

    #endif

    TT_LOG_I(TAG, "(%d) Started", device);
    return true;
}

bool start(spi_host_device_t device) {
    if (lock(device)) {
        bool result = startLocked(device);
        unlock(device);
        return result;
    } else {
        TT_LOG_E(TAG, "(%d) Mutex timeout", device);
        return false;
    }
}

static bool stopLocked(spi_host_device_t device) {
    Data& data = dataArray[device];
    Configuration& config = data.configuration;

    if (!config.canReinit) {
        TT_LOG_E(TAG, "(%d) Stopping: Not allowed to re-init", device);
        return false;
    }

    if (!data.isStarted) {
        TT_LOG_E(TAG, "(%d) Stopping: Not started", device);
        return false;
    }

    #ifdef ESP_PLATFORM

    auto result = spi_bus_free(device);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "(%d) Stopping: Failed to free device: %s", device, esp_err_to_name(result));
        return false;
    } else {
        data.isStarted = false;
    }

    #else

    data.isStarted = false;

    #endif

    TT_LOG_I(TAG, "(%d) Stopped", device);
    return true;
}

bool stop(spi_host_device_t device) {
    if (lock(device)) {
        bool result = stopLocked(device);
        unlock(device);
        return result;
    } else {
        TT_LOG_E(TAG, "(%d) Mutex timeout", device);
        return false;
    }
}

bool isStarted(spi_host_device_t device) {
    if (lock(device, 50 / portTICK_PERIOD_MS)) {
        bool started = dataArray[device].isStarted;
        unlock(device);
        return started;
    } else {
        // If we can't get a lock, we assume the device is busy and thus has started
        return true;
    }
}

bool lock(spi_host_device_t device, TickType_t timeout) {
    return dataArray[device].lock->lock(timeout);
}

bool unlock(spi_host_device_t device) {
    return dataArray[device].lock->unlock();
}

}
