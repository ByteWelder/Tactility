// SPDX-License-Identifier: Apache-2.0
#include <gps/private/gps_ledger.h>

#include <gps/gps.h>
#include <gps/gps_settings.h>
#include <tactility/concurrent/mutex.h>
#include <tactility/delay.h>
#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/log.h>

#include <cstdio>
#include <cstring>
#include <new>
#include <vector>

constexpr auto* TAG = "gps_ledger";
constexpr uint32_t DESTROY_RETRY_INTERVAL_MS = 10;
constexpr uint32_t DESTROY_TIMEOUT_MS = 10 * 1000;

struct GpsConfig {
    uint32_t baud_rate;
    enum GpsModel model;
};

// A GPS_TYPE device the ledger constructed for a persisted GpsConfiguration. Device is the first
// member so `reinterpret_cast<GpsLedgerEntry*>(device)` is safe when a Device* obtained from the
// device tree needs to be freed.
struct GpsLedgerEntry {
    Device device {};
    GpsConfig config {};
    // device->name points into this buffer - must outlive the device (device->name only stores a
    // pointer, it doesn't copy).
    char name[16] {};
};

// Syncs run on the app thread when settings change, and on the thread that starts or stops a UART
struct LedgerMutex {
    Mutex mutex {};
    LedgerMutex() { mutex_construct(&mutex); }
    ~LedgerMutex() { mutex_destruct(&mutex); }
};

static LedgerMutex ledger_mutex;

// Unique across every device the ledger creates, so device names ("gpsN") never collide.
static uint32_t next_device_index = 0;

static bool device_matches_configuration(Device* device, const GpsConfiguration& configuration) {
    auto* parent = device_get_parent(device);
    if (parent == nullptr || strcmp(parent->name, configuration.uart_name) != 0) {
        return false;
    }
    const auto* config = static_cast<const GpsConfig*>(device->config);
    return config->baud_rate == configuration.baud_rate && config->model == configuration.model;
}

// Constructs+adds (not started) a GPS_TYPE device wired to `configuration`'s named UART, tagged
// DEVICE_FLAG_DYNAMIC so the ledger recognizes it as its own on a later sync.
static bool create_device(const GpsConfiguration& configuration) {
    Device* uart = nullptr;
    if (device_get_by_name(configuration.uart_name, &uart) != ERROR_NONE) {
        LOG_W(TAG, "Device %s is not available", configuration.uart_name);
        return false;
    }

    auto* entry = new(std::nothrow) GpsLedgerEntry();
    if (entry == nullptr) {
        device_put(uart);
        return false;
    }
    entry->config = GpsConfig { .baud_rate = configuration.baud_rate, .model = configuration.model };
    snprintf(entry->name, sizeof(entry->name), "gps%u", (unsigned)next_device_index++);

    auto* device = &entry->device;
    device->address = 0;
    device->name = entry->name;
    device->config = &entry->config;
    device->parent = nullptr;
    device->flags = DEVICE_FLAG_DYNAMIC;
    device->internal = nullptr;

    bool ok = false;
    if (device_construct(device) == ERROR_NONE) {
        device_set_parent(device, uart);

        Driver* driver = driver_find_compatible("tactility,gps-generic");
        if (driver != nullptr) {
            device_set_driver(device, driver);
            ok = device_add(device) == ERROR_NONE;
            if (!ok) {
                LOG_E(TAG, "Failed to add %s", device->name);
            }
        } else {
            LOG_E(TAG, "No driver registered for tactility,gps-generic");
        }

        if (!ok) {
            device_destruct(device);
        }
    } else {
        LOG_E(TAG, "Failed to construct %s", device->name);
    }

    device_put(uart);

    if (!ok) {
        delete entry;
    }
    return ok;
}

// Stops (if needed), removes and destructs a ledger-owned device, and frees its entry.
// Another task can be starting or stopping the device, or hold a reference to it, so each step is retried until it succeeds.
// A device that can't be destructed in time is leaked, as freeing it could cause a use-after-free.
static void destroy_device(Device* device) {
    for (uint32_t waited_ms = 0; waited_ms < DESTROY_TIMEOUT_MS; waited_ms += DESTROY_RETRY_INTERVAL_MS) {
        if (device_is_added(device)) {
            const error_t stop_error = device_stop(device);
            if (stop_error == ERROR_RESOURCE) {
                break;
            }
            if (stop_error == ERROR_NONE) {
                device_remove(device);
            }
        }
        if (!device_is_added(device) && device_destruct(device) == ERROR_NONE) {
            delete reinterpret_cast<GpsLedgerEntry*>(device);
            return;
        }
        delay_millis(DESTROY_RETRY_INTERVAL_MS);
    }
    LOG_E(TAG, "Failed to destroy %s", device->name);
}

static bool is_ledger_owned(const Device* device) {
    return !(device->flags & DEVICE_FLAG_DTS) && (device->flags & DEVICE_FLAG_DYNAMIC);
}

// Collects the ledger-owned GPS_TYPE devices. device_remove()/device_stop() must not run while
// device_for_each_of_type() holds the device ledger lock, so callers process the result afterwards.
static std::vector<Device*> collect_owned_devices() {
    std::vector<Device*> owned;
    device_for_each_of_type(&GPS_TYPE, &owned, [](Device* device, void* context) {
        if (is_ledger_owned(device)) {
            static_cast<std::vector<Device*>*>(context)->push_back(device);
        }
        return true;
    });
    return owned;
}

void gps_ledger_sync() {
    mutex_lock(&ledger_mutex.mutex);

    std::vector<GpsConfiguration> configurations;
    gps_settings_for_each_configuration(&configurations, [](const GpsConfiguration* configuration, size_t, void* context) {
        static_cast<std::vector<GpsConfiguration>*>(context)->push_back(*configuration);
    });

    std::vector<bool> matched(configurations.size(), false);
    std::vector<Device*> stale;

    for (auto* device : collect_owned_devices()) {
        bool found = false;
        for (size_t i = 0; i < configurations.size(); i++) {
            if (!matched[i] && device_matches_configuration(device, configurations[i])) {
                matched[i] = true;
                found = true;
                break;
            }
        }
        if (!found) {
            stale.push_back(device);
        }
    }

    // Configuration disappeared - stop, destruct and drop the device that was created for it.
    for (auto* device : stale) {
        destroy_device(device);
    }

    // New configuration - create a (not started) device for it.
    for (size_t i = 0; i < configurations.size(); i++) {
        if (!matched[i]) {
            create_device(configurations[i]);
        }
    }

    mutex_unlock(&ledger_mutex.mutex);
}

void gps_ledger_clear() {
    mutex_lock(&ledger_mutex.mutex);
    for (auto* device : collect_owned_devices()) {
        destroy_device(device);
    }
    mutex_unlock(&ledger_mutex.mutex);
}

void gps_ledger_remove_children_of(Device* parent) {
    mutex_lock(&ledger_mutex.mutex);
    for (auto* device : collect_owned_devices()) {
        if (device_get_parent(device) == parent) {
            destroy_device(device);
        }
    }
    mutex_unlock(&ledger_mutex.mutex);
}
