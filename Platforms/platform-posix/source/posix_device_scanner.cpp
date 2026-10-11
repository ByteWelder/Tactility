// SPDX-License-Identifier: Apache-2.0

#include <service/instance.h>
#include <service/manifest.h>
#include <tactility/concurrent/mutex.h>
#include <tactility/concurrent/timer.h>
#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/drivers/posix_i2c.h>
#include <tactility/drivers/posix_uart.h>
#include <tactility/log.h>
#include <tactility/time.h>

#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <new>
#include <set>
#include <string>
#include <unistd.h>
#include <vector>

#define TAG "posix_device_scanner"

namespace {

constexpr auto* DEVICE_DIRECTORY = "/dev";
constexpr uint32_t SCAN_INTERVAL_MS = 1000;

constexpr auto* UART_COMPATIBLE = "posix,uart";
constexpr auto* I2C_COMPATIBLE = "posix,i2c";

struct DevicePattern {
    const char* prefix;
    const char* compatible;
};

constexpr DevicePattern DEVICE_PATTERNS[] = {
    { "ttyUSB", UART_COMPATIBLE },
    { "ttyACM", UART_COMPATIBLE },
    { "cu.usbserial", UART_COMPATIBLE },
    { "cu.usbmodem", UART_COMPATIBLE },
    { "i2c-", I2C_COMPATIBLE },
};

struct ScannedDevice {
    std::string path;
    std::string name;
    PosixUartConfig uart_config {};
    PosixI2cConfig i2c_config {};
    Device device {};
    bool pending_removal = false;
    bool busy_logged = false;
};

struct ScannerData {
    Mutex mutex {};
    Timer* timer = nullptr;
    bool stopping = false;
    std::vector<ScannedDevice*> devices;
    /** Paths that failed to register, so the failure is only logged once */
    std::set<std::string> failed_paths;
};

const char* find_compatible(const char* file_name) {
    for (const auto& pattern : DEVICE_PATTERNS) {
        if (strncmp(file_name, pattern.prefix, strlen(pattern.prefix)) == 0) {
            return pattern.compatible;
        }
    }
    return nullptr;
}

bool device_name_exists(const char* name) {
    Device* existing = nullptr;
    error_t error = device_get_by_name(name, &existing);
    if (error == ERROR_NONE) {
        device_put(existing);
    }
    return error != ERROR_NOT_FOUND;
}

/** @return the registered device, or nullptr on failure */
ScannedDevice* register_device(const std::string& file_name, const char* compatible, bool log_failure) {
    auto* driver = driver_find_compatible(compatible);
    if (driver == nullptr) {
        if (log_failure) LOG_E(TAG, "No driver for %s", compatible);
        return nullptr;
    }

    if (device_name_exists(file_name.c_str())) {
        if (log_failure) LOG_W(TAG, "Skipping %s: a device with that name already exists", file_name.c_str());
        return nullptr;
    }

    Device* root = nullptr;
    if (device_get_by_name("/", &root) != ERROR_NONE) {
        return nullptr;
    }

    auto* scanned = new(std::nothrow) ScannedDevice();
    if (scanned == nullptr) {
        device_put(root);
        return nullptr;
    }
    scanned->path = std::string(DEVICE_DIRECTORY) + "/" + file_name;
    scanned->name = file_name;
    scanned->uart_config.path = scanned->path.c_str();
    scanned->i2c_config.path = scanned->path.c_str();
    scanned->device.name = scanned->name.c_str();
    scanned->device.config = (strcmp(compatible, UART_COMPATIBLE) == 0)
        ? static_cast<const void*>(&scanned->uart_config)
        : static_cast<const void*>(&scanned->i2c_config);

    Device* device = &scanned->device;
    if (device_construct(device) != ERROR_NONE) {
        device_put(root);
        delete scanned;
        return nullptr;
    }
    device_set_parent(device, root);
    device_put(root);
    device_set_driver(device, driver);

    if (device_add(device) != ERROR_NONE) {
        if (log_failure) LOG_E(TAG, "Failed to add %s", scanned->path.c_str());
        device_destruct(device);
        delete scanned;
        return nullptr;
    }

    error_t start_error = device_start(device);
    if (start_error != ERROR_NONE) {
        if (log_failure) LOG_E(TAG, "Failed to start %s: %s", scanned->path.c_str(), error_to_string(start_error));
        device_remove(device);
        device_destruct(device);
        delete scanned;
        return nullptr;
    }

    LOG_I(TAG, "Registered %s (%s)", scanned->name.c_str(), compatible);
    return scanned;
}

/** @return true when the device was unregistered and freed */
bool unregister_device(ScannedDevice* scanned) {
    Device* device = &scanned->device;
    // A previous attempt can have removed the device already, when destructing it failed
    if (device_is_added(device)) {
        error_t stop_error = device_stop(device);
        if (stop_error != ERROR_NONE) {
            if (!scanned->busy_logged) {
                LOG_W(TAG, "%s is in use, retrying removal: %s", scanned->name.c_str(), error_to_string(stop_error));
                scanned->busy_logged = true;
            }
            return false;
        }
        device_remove(device);
    }
    // Fails while other devices are still children of this device
    error_t destruct_error = device_destruct(device);
    if (destruct_error != ERROR_NONE) {
        if (!scanned->busy_logged) {
            LOG_W(TAG, "%s can't be destructed yet, retrying removal: %s", scanned->name.c_str(), error_to_string(destruct_error));
            scanned->busy_logged = true;
        }
        return false;
    }
    LOG_I(TAG, "Unregistered %s", scanned->name.c_str());
    delete scanned;
    return true;
}

std::set<std::string> list_device_files() {
    std::set<std::string> result;
    DIR* dir = opendir(DEVICE_DIRECTORY);
    if (dir == nullptr) return result;
    while (dirent* entry = readdir(dir)) {
        if (find_compatible(entry->d_name) != nullptr) {
            result.insert(entry->d_name);
        }
    }
    closedir(dir);
    return result;
}

void scan(ScannerData* data) {
    auto files = list_device_files();

    // Removals, including retries of devices that were still in use
    for (auto* scanned : data->devices) {
        bool present = files.contains(scanned->name);
        if (present && scanned->pending_removal && device_is_added(&scanned->device)) {
            scanned->pending_removal = false;
            scanned->busy_logged = false;
        } else if (!present) {
            scanned->pending_removal = true;
        }
    }
    std::erase_if(data->devices, [](ScannedDevice* scanned) {
        return scanned->pending_removal && unregister_device(scanned);
    });

    // Additions
    for (const auto& file_name : files) {
        bool tracked = std::ranges::any_of(data->devices, [&file_name](ScannedDevice* scanned) {
            return scanned->name == file_name;
        });
        if (tracked) continue;

        bool log_failure = !data->failed_paths.contains(file_name);
        std::string path = std::string(DEVICE_DIRECTORY) + "/" + file_name;
        if (access(path.c_str(), R_OK | W_OK) != 0) {
            if (log_failure) LOG_W(TAG, "Skipping %s: no read/write access", path.c_str());
            data->failed_paths.insert(file_name);
            continue;
        }

        auto* scanned = register_device(file_name, find_compatible(file_name.c_str()), log_failure);
        if (scanned != nullptr) {
            data->devices.push_back(scanned);
            data->failed_paths.erase(file_name);
        } else {
            data->failed_paths.insert(file_name);
        }
    }

    // Forget failures of files that disappeared, so a new file with that name logs again
    std::erase_if(data->failed_paths, [&files](const std::string& file_name) {
        return !files.contains(file_name);
    });
}

void on_timer(void* context) {
    auto* data = static_cast<ScannerData*>(context);
    mutex_lock(&data->mutex);
    if (!data->stopping) {
        scan(data);
    }
    mutex_unlock(&data->mutex);
}

// ---- Service ----

void* create_service(const ServiceManifest*) {
    auto* data = new(std::nothrow) ScannerData();
    if (data == nullptr) return nullptr;
    mutex_construct(&data->mutex);
    return data;
}

void destroy_service(const ServiceManifest*, void* service_data) {
    auto* data = static_cast<ScannerData*>(service_data);
    mutex_destruct(&data->mutex);
    delete data;
}

error_t on_start(ServiceInstance*, void* service_data) {
    auto* data = static_cast<ScannerData*>(service_data);
    data->stopping = false;
    data->timer = timer_alloc(TIMER_TYPE_PERIODIC, millis_to_ticks(SCAN_INTERVAL_MS), on_timer, data);
    if (data->timer == nullptr) return ERROR_OUT_OF_MEMORY;

    on_timer(data);

    error_t error = timer_start(data->timer);
    if (error != ERROR_NONE) {
        timer_free(data->timer);
        data->timer = nullptr;
    }
    return error;
}

void on_stop(ServiceInstance*, void* service_data) {
    auto* data = static_cast<ScannerData*>(service_data);

    mutex_lock(&data->mutex);
    data->stopping = true;
    mutex_unlock(&data->mutex);

    timer_stop(data->timer);
    timer_free(data->timer);
    data->timer = nullptr;

    mutex_lock(&data->mutex);
    for (auto* scanned : data->devices) {
        // A device that is still in use stays registered, so its memory must stay valid
        if (!unregister_device(scanned)) {
            LOG_E(TAG, "%s is still in use and stays registered", scanned->name.c_str());
        }
    }
    data->devices.clear();
    data->failed_paths.clear();
    mutex_unlock(&data->mutex);
}

} // namespace

extern "C" {

extern const ServiceManifest posix_device_scanner_service_manifest;

const ServiceManifest posix_device_scanner_service_manifest = {
    .id = "posix-device-scanner",
    .create_service = create_service,
    .destroy_service = destroy_service,
    .on_start = on_start,
    .on_stop = on_stop,
};

}
