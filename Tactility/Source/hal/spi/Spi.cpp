#include <Tactility/hal/spi/Spi.h>

#include <Tactility/Logger.h>
#include <Tactility/RecursiveMutex.h>

namespace tt::hal::spi {

static const auto LOGGER = Logger("SPI");

struct Data {
    std::shared_ptr<Lock> lock;
    bool isConfigured = false;
    bool isStarted = false;
    Configuration configuration;
};

static Data dataArray[SPI_HOST_MAX];

bool init(const std::vector<Configuration>& configurations) {
    LOGGER.info("Init");
    for (const auto& configuration: configurations) {
        Data& data = dataArray[configuration.device];
        data.configuration = configuration;
        data.isConfigured = true;
        if (configuration.lock != nullptr) {
            data.lock = configuration.lock;
        } else {
            data.lock = std::make_shared<RecursiveMutex>();
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
        LOGGER.error("({}) Cannot reconfigure while interface is started", static_cast<int>(device));
        return false;
    } else if (!data.configuration.isMutable) {
        LOGGER.error("({}) Mutation not allowed by original configuration", static_cast<int>(device));
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
        LOGGER.error("({}) Starting: Already started", static_cast<int>(device));
        return false;
    }

    if (!data.isConfigured) {
        LOGGER.error("({}) Starting: Not configured", static_cast<int>(device));
        return false;
    }

#ifdef ESP_PLATFORM

    auto result = spi_bus_initialize(device, &data.configuration.config, data.configuration.dma);
    if (result != ESP_OK) {
        LOGGER.error("({}) Starting: Failed to initialize: {}", static_cast<int>(device), esp_err_to_name(result));
        return false;
    } else {
        data.isStarted = true;
    }

#else

    data.isStarted = true;

#endif

    LOGGER.info("({}) Started", static_cast<int>(device));
    return true;
}

bool stop(spi_host_device_t device) {
    auto lock = getLock(device)->asScopedLock();
    lock.lock();

    Data& data = dataArray[device];
    Configuration& config = data.configuration;

    if (!config.isMutable) {
        LOGGER.error("({}) Stopping: Not allowed, immutable", static_cast<int>(device));
        return false;
    }

    if (!data.isStarted) {
        LOGGER.error("({}) Stopping: Not started", static_cast<int>(device));
        return false;
    }

#ifdef ESP_PLATFORM

    auto result = spi_bus_free(device);
    if (result != ESP_OK) {
        LOGGER.error("({}) Stopping: Failed to free device: {}", static_cast<int>(device), esp_err_to_name(result));
        return false;
    } else {
        data.isStarted = false;
    }

#else

    data.isStarted = false;

#endif

    LOGGER.info("({}) Stopped", static_cast<int>(device));
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
