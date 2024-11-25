#ifndef ESP_TARGET

/**
 * This code is based on i2c_manager from https://github.com/ropg/i2c_manager/blob/master/i2c_manager/i2c_manager.c (original has MIT license)
 */
#include "I2c.h"
#include "Log.h"
#include "Mutex.h"

namespace tt::hal::i2c {

typedef struct Data {
    Mutex mutex;
    bool isConfigured = false;
    bool isStarted = false;
    Configuration configuration;
} Data;

static Data dataArray[I2C_NUM_MAX];

#define TAG "i2c"

bool init(const std::vector<i2c::Configuration>& configurations) {
   TT_LOG_I(TAG, "Init");
   for (const auto& configuration: configurations) {
       Data& data = dataArray[configuration.port];
       data.configuration = configuration;
       data.isConfigured = true;
   }

   for (const auto& config: configurations) {
       if (config.initMode == InitByTactility && !start(config.port)) {
           return false;
       } else if (config.initMode == InitByExternal) {
           dataArray[config.port].isStarted = true;
       }
   }

   return true;
}

static bool configureLocked(i2c_port_t port, const i2c_config_t& configuration) {
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

esp_err_t configure(i2c_port_t port, const i2c_config_t& configuration) {
    lock(port);
    bool result = configureLocked(port, configuration);
    unlock(port);
    return result;
}

bool start(i2c_port_t port) {
    lock(port);
    dataArray[port].isStarted = true;
    unlock(port);
    return true;
}

bool stop(i2c_port_t port) {
    lock(port);
    dataArray[port].isStarted = false;
    unlock(port);
    return true;
}

bool isStarted(i2c_port_t port) {
    lock(port);
    bool started = dataArray[port].isStarted;
    unlock(port);
    return started;
}

bool read(i2c_port_t port, uint16_t address, uint32_t reg, uint8_t* buffer, uint16_t size) {
    return false;
}

bool write(i2c_port_t port, uint16_t address, uint32_t reg, const uint8_t* buffer, uint16_t size) {
    return false;
}

TtStatus lock(i2c_port_t port, uint32_t timeout) {
    return dataArray[port].mutex.acquire(timeout);
}

TtStatus unlock(i2c_port_t port) {
    return dataArray[port].mutex.release();
}

} // namespace

#endif