#include "Tactility/hal/i2c/I2c.h"

#include <Tactility/Log.h>
#include <Tactility/Mutex.h>

#ifdef ESP_PLATFORM
#include <esp_check.h>
#endif // ESP_PLATFORM

#define TAG "i2c"

namespace tt::hal::i2c {

struct Data {
    Mutex mutex;
    bool isConfigured = false;
    bool isStarted = false;
    Configuration configuration;
};

static const uint8_t ACK_CHECK_EN = 1;
static Data dataArray[I2C_NUM_MAX];

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
    TT_LOG_V(TAG, "I2C info for port %d", data.configuration.port);
    TT_LOG_V(TAG, "  isStarted: %d", data.isStarted);
    TT_LOG_V(TAG, "  isConfigured: %d", data.isConfigured);
    TT_LOG_V(TAG, "  initMode: %s", toString(data.configuration.initMode));
    TT_LOG_V(TAG, "  canReinit: %d", data.configuration.canReinit);
    TT_LOG_V(TAG, "  hasMutableConfiguration: %d", data.configuration.hasMutableConfiguration);
#ifdef ESP_PLATFORM
    TT_LOG_V(TAG, "  SDA pin: %d", data.configuration.config.sda_io_num);
    TT_LOG_V(TAG, "  SCL pin: %d", data.configuration.config.scl_io_num);
#endif // ESP_PLATFORM
}

bool init(const std::vector<i2c::Configuration>& configurations) {
   TT_LOG_I(TAG, "Init");
   for (const auto& configuration: configurations) {
#ifdef ESP_PLATFORM
       if (configuration.config.mode != I2C_MODE_MASTER) {
           TT_LOG_E(TAG, "Currently only master mode is supported");
           return false;
       }
#endif // ESP_PLATFORM
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

bool configure(i2c_port_t port, const i2c_config_t& configuration) {
    auto lockable = getLock(port).asScopedLock();
    lockable.lock();

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

bool start(i2c_port_t port) {
    auto lockable = getLock(port).asScopedLock();
    lockable.lock();

    Data& data = dataArray[port];
    printInfo(data);
    Configuration& config = data.configuration;

    if (data.isStarted) {
        TT_LOG_E(TAG, "(%d) Starting: Already started", port);
        return false;
    }

    if (!data.isConfigured) {
        TT_LOG_E(TAG, "(%d) Starting: Not configured", port);
        return false;
    }

#ifdef ESP_PLATFORM
    esp_err_t result = i2c_param_config(port, &config.config);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "(%d) Starting: Failed to configure: %s", port, esp_err_to_name(result));
        return false;
    }

    result = i2c_driver_install(port, config.config.mode, 0, 0, 0);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "(%d) Starting: Failed to install driver: %s", port, esp_err_to_name(result));
        return false;
    }
#endif // ESP_PLATFORM

    data.isStarted = true;

    TT_LOG_I(TAG, "(%d) Started", port);
    return true;
}

bool stop(i2c_port_t port) {
    auto lockable = getLock(port).asScopedLock();
    lockable.lock();

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
    esp_err_t result = i2c_driver_delete(port);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "(%d) Stopping: Failed to delete driver: %s", port, esp_err_to_name(result));
        return false;
    }
#endif // ESP_PLATFORM

    data.isStarted = false;

    TT_LOG_I(TAG, "(%d) Stopped", port);
    return true;
}

bool isStarted(i2c_port_t port) {
    auto lockable = getLock(port).asScopedLock();
    lockable.lock();
    return dataArray[port].isStarted;
}

bool masterRead(i2c_port_t port, uint8_t address, uint8_t* data, size_t dataSize, TickType_t timeout) {
    auto lockable = getLock(port).asScopedLock();
    if (!lockable.lock(timeout)) {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }

#ifdef ESP_PLATFORM
    auto result = i2c_master_read_from_device(port, address, data, dataSize, timeout);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    return result == ESP_OK;
#else
    return false;
#endif // ESP_PLATFORM
}

bool masterReadRegister(i2c_port_t port, uint8_t address, uint8_t reg, uint8_t* data, size_t dataSize, TickType_t timeout) {
    auto lockable = getLock(port).asScopedLock();
    if (!lockable.lock(timeout)) {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }

#ifdef ESP_PLATFORM
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    // Set address pointer
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write(cmd, &reg, 1, ACK_CHECK_EN);
    // Read length of response from current pointer
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
    if (dataSize > 1) {
        i2c_master_read(cmd, data, dataSize - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + dataSize - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    // TODO: We're passing an inaccurate timeout value as we already lost time with locking
    esp_err_t result = i2c_master_cmd_begin(port, cmd, timeout);
    i2c_cmd_link_delete(cmd);

    ESP_ERROR_CHECK_WITHOUT_ABORT(result);

    return result == ESP_OK;
#else
    return false;
#endif // ESP_PLATFORM
}

bool masterWrite(i2c_port_t port, uint8_t address, const uint8_t* data, uint16_t dataSize, TickType_t timeout) {
    auto lockable = getLock(port).asScopedLock();
    if (!lockable.lock(timeout)) {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }

#ifdef ESP_PLATFORM
    auto result = i2c_master_write_to_device(port, address, data, dataSize, timeout);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    return result == ESP_OK;
#else
    return false;
#endif // ESP_PLATFORM
}

bool masterWriteRegister(i2c_port_t port, uint8_t address, uint8_t reg, const uint8_t* data, uint16_t dataSize, TickType_t timeout) {
    tt_check(reg != 0);

    auto lockable = getLock(port).asScopedLock();
    if (!lockable.lock(timeout)) {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }

#ifdef ESP_PLATFORM

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_write(cmd, (uint8_t*) data, dataSize, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    // TODO: We're passing an inaccurate timeout value as we already lost time with locking
    esp_err_t result = i2c_master_cmd_begin(port, cmd, timeout);
    i2c_cmd_link_delete(cmd);

    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    return result == ESP_OK;
#else
    return false;
#endif // ESP_PLATFORM
}

bool masterWriteRegisterArray(i2c_port_t port, uint8_t address, const uint8_t* data, uint16_t dataSize, TickType_t timeout) {
#ifdef ESP_PLATFORM
    assert(dataSize % 2 == 0);
    bool result = true;
    for (int i = 0; i < dataSize; i += 2) {
        // TODO: We're passing an inaccurate timeout value as we already lost time with locking and previous writes in this loop
        if (!masterWriteRegister(port, address, data[i], &data[i + 1], 1, timeout)) {
            result = false;
        }
    }
    return result;
#else
    return false;
#endif // ESP_PLATFORM
}

bool masterWriteRead(i2c_port_t port, uint8_t address, const uint8_t* writeData, size_t writeDataSize, uint8_t* readData, size_t readDataSize, TickType_t timeout) {
    auto lockable = getLock(port).asScopedLock();
    if (!lockable.lock(timeout)) {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }

#ifdef ESP_PLATFORM
    esp_err_t result = i2c_master_write_read_device(port, address, writeData, writeDataSize, readData, readDataSize, timeout);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    return result == ESP_OK;
#else
    return false;
#endif // ESP_PLATFORM
}

bool masterHasDeviceAtAddress(i2c_port_t port, uint8_t address, TickType_t timeout) {
    auto lockable = getLock(port).asScopedLock();
    if (!lockable.lock(timeout)) {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }

#ifdef ESP_PLATFORM
    uint8_t message[2] = { 0, 0 };
    // TODO: We're passing an inaccurate timeout value as we already lost time with locking
    return i2c_master_write_to_device(port, address, message, 2, timeout) == ESP_OK;
#else
    return false;
#endif // ESP_PLATFORM
}

Lockable& getLock(i2c_port_t port) {
    return dataArray[port].mutex;
}

} // namespace
