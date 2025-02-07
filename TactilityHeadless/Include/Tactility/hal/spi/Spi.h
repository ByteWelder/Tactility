#pragma once

#include "SpiCompat.h"

#include <Tactility/Lockable.h>
#include <Tactility/RtosCompat.h>

#include <vector>
#include <memory>

namespace tt::hal::spi {

enum class InitMode {
    ByTactility, // Tactility will initialize it in the correct bootup phase
    ByExternal, // The device is already initialized and Tactility should assume it works
    Disabled // Not initialized by default
};

struct Configuration {
    spi_host_device_t device;
    spi_common_dma_t dma;
    spi_bus_config_t config;
    /** Whether this bus should be initialized when device starts up */
    InitMode initMode;
    /** Whether this bus can stopped and re-started. */
    bool canReinit;
    /** Whether configuration can be changed. */
    bool hasMutableConfiguration;
    /** Optional custom lock */
    std::shared_ptr<Lockable> _Nullable lock;
};

enum class Status {
    Started,
    Stopped,
    Unknown
};

bool init(const std::vector<spi::Configuration>& configurations);

bool start(spi_host_device_t device);
bool stop(spi_host_device_t device);
bool isStarted(spi_host_device_t device);

bool lock(spi_host_device_t device, TickType_t timeout = 10 / portTICK_PERIOD_MS);
bool unlock(spi_host_device_t device);

} // namespace tt::hal::spi
