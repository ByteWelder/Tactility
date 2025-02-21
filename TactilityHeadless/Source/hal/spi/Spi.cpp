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

static const char* toString(InitMode mode) {
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
    TT_LOG_D(TAG, "SPI info for device %d", data.configuration.device);
    TT_LOG_D(TAG, "  isStarted: %d", data.isStarted);
    TT_LOG_D(TAG, "  isConfigured: %d", data.isConfigured);
    TT_LOG_D(TAG, "  initMode: %s", toString(data.configuration.initMode));
    TT_LOG_D(TAG, "  canReinit: %d", data.configuration.canReinit);
    TT_LOG_D(TAG, "  hasMutableConfiguration: %d", data.configuration.hasMutableConfiguration);
    TT_LOG_D(TAG, "  MISO pin: %d", data.configuration.config.miso_io_num);
    TT_LOG_D(TAG, "  MOSI pin: %d", data.configuration.config.mosi_io_num);
    TT_LOG_D(TAG, "  SCLK pin: %d", data.configuration.config.sclk_io_num);
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

bool configure(spi_host_device_t device, const spi_bus_config_t& configuration) {
    auto lock = getLock(device).asScopedLock();
    lock.lock();

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

bool start(spi_host_device_t device) {
    auto lock = getLock(device).asScopedLock();
    lock.lock();

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

bool stop(spi_host_device_t device) {
    auto lock = getLock(device).asScopedLock();
    lock.lock();

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

bool isStarted(spi_host_device_t device) {
    auto lock = getLock(device).asScopedLock();
    lock.lock();

    Data& data = dataArray[device];
    Configuration& config = data.configuration;

    return dataArray[device].isStarted;
}

Lock& getLock(spi_host_device_t device) {
    return *dataArray[device].lock;
}

}
