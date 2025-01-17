#ifdef ESP_TARGET

#include "I2c.h"
#include "Log.h"
#include "Mutex.h"

#include <esp_check.h>

namespace tt::hal::i2c {

static const uint8_t ACK_CHECK_EN = 1;

typedef struct Data {
    Mutex mutex;
    bool isConfigured = false;
    bool isStarted = false;
    Configuration configuration;
} Data;

static Data dataArray[I2C_NUM_MAX];

#define TAG "i2c"

const char* initModeToString(InitMode mode) {
    switch (mode) {
        case InitMode::ByTactility:
            return TT_STRINGIFY(InitMode::ByTactility);
        case InitMode::ByExternal:
            return TT_STRINGIFY(InitMode::ByExternal);
        case InitMode::Disabled:
            return TT_STRINGIFY(InitMode::Disabled);
    }
    tt_crash("not implemented");
}

void printInfo(const Data& data) {
    TT_LOG_V(TAG, "I2C info for port %d", data.configuration.port);
    TT_LOG_V(TAG, "  isStarted: %d", data.isStarted);
    TT_LOG_V(TAG, "  isConfigured: %d", data.isConfigured);
    TT_LOG_V(TAG, "  initMode: %s", initModeToString(data.configuration.initMode));
    TT_LOG_V(TAG, "  canReinit: %d", data.configuration.canReinit);
    TT_LOG_V(TAG, "  hasMutableConfiguration: %d", data.configuration.hasMutableConfiguration);
    TT_LOG_V(TAG, "  SDA pin: %d", data.configuration.config.sda_io_num);
    TT_LOG_V(TAG, "  SCL pin: %d", data.configuration.config.scl_io_num);
}

bool init(const std::vector<i2c::Configuration>& configurations) {
   TT_LOG_I(TAG, "Init");
   for (const auto& configuration: configurations) {
       if (configuration.config.mode != I2C_MODE_MASTER) {
           TT_LOG_E(TAG, "Currently only master mode is supported");
           return false;
       }
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

static bool configureLocked(i2c_port_t port, const i2c_config_t& configuration) {
    Data& data = dataArray[port];
    if (data.isStarted) {
        TT_LOG_E(TAG, "(%d) Cannot reconfigure while interface is started", port);
        return ESP_ERR_INVALID_STATE;
    } else if (!data.configuration.hasMutableConfiguration) {
        TT_LOG_E(TAG, "(%d) Mutation not allowed by original configuration", port);
        return ESP_ERR_NOT_ALLOWED;
    } else {
        data.configuration.config = configuration;
        return ESP_OK;
    }
}

bool configure(i2c_port_t port, const i2c_config_t& configuration) {
    if (lock(port)) {
        bool result = configureLocked(port, configuration);
        unlock(port);
        return result;
    } else {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }
}

static bool startLocked(i2c_port_t port) {
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

    esp_err_t result = i2c_param_config(port, &config.config);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "(%d) Starting: Failed to configure: %s", port, esp_err_to_name(result));
        return false;
    }

    result = i2c_driver_install(port, config.config.mode, 0, 0, 0);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "(%d) Starting: Failed to install driver: %s", port, esp_err_to_name(result));
        return false;
    } else {
        data.isStarted = true;
    }

    TT_LOG_I(TAG, "(%d) Started", port);
    return true;
}

bool start(i2c_port_t port) {
    if (lock(port)) {
        bool result = startLocked(port);
        unlock(port);
        return result;
    } else {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }
}

static bool stopLocked(i2c_port_t port) {
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

    esp_err_t result = i2c_driver_delete(port);
    if (result != ESP_OK) {
        TT_LOG_E(TAG, "(%d) Stopping: Failed to delete driver: %s", port, esp_err_to_name(result));
        return false;
    } else {
        data.isStarted = false;
    }

    TT_LOG_I(TAG, "(%d) Stopped", port);
    return true;
}

bool stop(i2c_port_t port) {
    if (lock(port)) {
        bool result = stopLocked(port);
        unlock(port);
        return result;
    } else {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }
}

bool isStarted(i2c_port_t port) {
    if (lock(port, 50 / portTICK_PERIOD_MS)) {
        bool started = dataArray[port].isStarted;
        unlock(port);
        return started;
    } else {
        // If we can't get a lock, we assume the device is busy and thus has started
        return true;
    }
}

bool masterRead(i2c_port_t port, uint8_t address, uint8_t* data, size_t dataSize, TickType_t timeout) {
    if (lock(port)) {
        // TODO: We're passing an inaccurate timeout value as we already lost time with locking and previous writes in this loop
        esp_err_t result = i2c_master_read_from_device(port, address, data, dataSize, timeout);
        unlock(port);
        return result == ESP_OK;
    } else {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }
}

bool masterReadRegister(i2c_port_t port, uint8_t address, uint8_t reg, uint8_t* data, size_t dataSize, TickType_t timeout) {
    if (!lock(port)) {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }

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

    unlock(port);

    ESP_LOG_BUFFER_HEX_LEVEL(TAG, data, dataSize, ESP_LOG_DEBUG);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);

    return result == ESP_OK;
}

bool masterWrite(i2c_port_t port, uint8_t address, const uint8_t* data, uint16_t dataSize, TickType_t timeout) {
    if (lock(port)) {
        // TODO: We're passing an inaccurate timeout value as we already lost time with locking
        esp_err_t result = i2c_master_write_to_device(port, address, data, dataSize, timeout);
        unlock(port);
        return result == ESP_OK;
    } else {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }
}

bool masterWriteRegister(i2c_port_t port, uint8_t address, uint8_t reg, const uint8_t* data, uint16_t dataSize, TickType_t timeout) {
    tt_check(reg != 0);

    if (!lock(port)) {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_write(cmd, (uint8_t*) data, dataSize, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    // TODO: We're passing an inaccurate timeout value as we already lost time with locking
    esp_err_t result = i2c_master_cmd_begin(port, cmd, timeout);
    i2c_cmd_link_delete(cmd);

    unlock(port);

    ESP_ERROR_CHECK_WITHOUT_ABORT(result);

    return result == ESP_OK;
}

bool masterWriteRegisterArray(i2c_port_t port, uint8_t address, const uint8_t* data, uint16_t dataSize, TickType_t timeout) {
    assert(dataSize % 2 == 0);
    bool result = true;
    for (int i = 0; i < dataSize; i += 2) {
        // TODO: We're passing an inaccurate timeout value as we already lost time with locking and previous writes in this loop
        if (!masterWriteRegister(port, address, data[i], &data[i + 1], 1, timeout)) {
            result = false;
        }
    }
    return result;
}

bool masterWriteRead(i2c_port_t port, uint8_t address, const uint8_t* writeData, size_t writeDataSize, uint8_t* readData, size_t readDataSize, TickType_t timeout) {
    if (lock(port)) {
        // TODO: We're passing an inaccurate timeout value as we already lost time with locking
        esp_err_t result = i2c_master_write_read_device(port, address, writeData, writeDataSize, readData, readDataSize, timeout);
        unlock(port);
        return result == ESP_OK;
    } else {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }
}

bool masterHasDeviceAtAddress(i2c_port_t port, uint8_t address, TickType_t timeout) {
    if (lock(port)) {
        uint8_t message[2] = { 0, 0 };
        // TODO: We're passing an inaccurate timeout value as we already lost time with locking
        esp_err_t result = i2c_master_write_to_device(port, address, message, 2, timeout);
        unlock(port);
        return result == ESP_OK;
    } else {
        TT_LOG_E(TAG, "(%d) Mutex timeout", port);
        return false;
    }
}

bool lock(i2c_port_t port, TickType_t timeout) {
    return dataArray[port].mutex.lock(timeout);
}

bool unlock(i2c_port_t port) {
    return dataArray[port].mutex.unlock();
}

} // namespace

#endif