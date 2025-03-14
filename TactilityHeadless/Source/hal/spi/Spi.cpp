#include "Tactility/hal/spi/Spi.h"

#include <Tactility/Mutex.h>

#define TAG "spi"

namespace tt::hal::spi {

struct Data {
    std::shared_ptr<Lock> lock;
    bool isConfigured = false;
    bool isStarted = false;
    Configuration configuration;
};

static Data dataArray[SPI_HOST_MAX];

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

bool configure(spi_host_device_t device, const spi_bus_config_t& configuration) {
    auto lock = getLock(device)->asScopedLock();
    lock.lock();

    Data& data = dataArray[device];
    if (data.isStarted) {
        TT_LOG_E(TAG, "(%d) Cannot reconfigure while interface is started", device);
        return false;
    } else if (!data.configuration.isMutable) {
        TT_LOG_E(TAG, "(%d) Mutation not allowed by original configuration", device);
        return false;
    } else {
        data.configuration.config = configuration;
        return true;
    }
}

bool start(spi_host_device_t device) {
    auto lock = getLock(device)->asScopedLock();
    lock.lock();

    Data& data = dataArray[device];

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

bool stop(spi_host_device_t device) {
    auto lock = getLock(device)->asScopedLock();
    lock.lock();

    Data& data = dataArray[device];
    Configuration& config = data.configuration;

    if (!config.isMutable) {
        TT_LOG_E(TAG, "(%d) Stopping: Not allowed, immutable", device);
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

bool isStarted(spi_host_device_t device) {
    auto lock = getLock(device)->asScopedLock();
    lock.lock();

    return dataArray[device].isStarted;
}

std::shared_ptr<Lock> getLock(spi_host_device_t device) {
    return dataArray[device].lock;
}

}
