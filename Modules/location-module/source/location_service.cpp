// SPDX-License-Identifier: Apache-2.0
#include <location/location.h>
#include <location/location_service.h>
#include <location/location_settings.h>
#include <location/provider.h>

#include <service/manager.h>
#include <service/manifest.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/concurrent/task_event_group.h>
#include <tactility/log.h>
#include <tactility/time.h>

#include <cmath>
#include <cstring>

#define TAG "location"

struct LocationHost {
    // Guarded by lifecycleMutex
    const LocationProviderApi* api;
    void* provider;

    // Guarded by dataMutex
    bool fallback;
    bool hasLocation;
    Location location;
};

namespace {

/** A fix of a provider that isn't a fallback is preferred over fallback fixes for this long */
constexpr uint64_t FRESH_MS = 120000;
// About 2 km north to south
constexpr double APPROXIMATE_GRID_DEGREES = 0.02;

struct GlobalMutex {
    Mutex handle {};
    GlobalMutex() { mutex_construct(&handle); }
    ~GlobalMutex() { mutex_destruct(&handle); }
};

// Serializes provider registration, service start/stop and (un)subscribing, which create and destroy providers
GlobalMutex lifecycleMutex;
LocationHost hosts[LOCATION_PROVIDER_MAX_COUNT] {};
bool running = false;
// Providers only exist while the service runs and has subscribers
bool providersActive = false;

// Not held while calling provider functions, so providers can report while being destroyed
GlobalMutex dataMutex;
LocationSubscription* subscriptions = nullptr;

GlobalMutex settingsMutex;
LocationSettings settings {};
bool settingsLoaded = false;

/** Must be called with settingsMutex held */
void ensureSettingsLoaded() {
    if (!settingsLoaded) {
        location_settings_load(&settings);
        settingsLoaded = true;
    }
}

uint64_t getUptimeMs() {
    return get_micros_since_boot() / 1000;
}

/** Must be called with dataMutex held */
int getRank(const LocationHost& host, uint64_t now) {
    if (host.fallback) {
        return 1;
    }
    return now - host.location.uptime_ms < FRESH_MS ? 2 : 0;
}

/** Must be called with dataMutex held */
const LocationHost* findBest() {
    const uint64_t now = getUptimeMs();
    const LocationHost* best = nullptr;
    int bestRank = -1;
    for (const auto& host : hosts) {
        if (!host.hasLocation) {
            continue;
        }
        const int rank = getRank(host, now);
        if (rank > bestRank || (rank == bestRank && host.location.uptime_ms > best->location.uptime_ms)) {
            best = &host;
            bestRank = rank;
        }
    }
    return best;
}

void clearLocation(LocationHost* host) {
    mutex_lock(&dataMutex.handle);
    host->hasLocation = false;
    mutex_unlock(&dataMutex.handle);
}

void startProvider(LocationHost* host) {
    host->provider = host->api->create(host);
    if (host->provider == nullptr) {
        LOG_E(TAG, "Failed to start provider %s", host->api->id);
    }
}

void stopProvider(LocationHost* host) {
    if (host->provider != nullptr) {
        host->api->destroy(host->provider);
        host->provider = nullptr;
    }
}

/** Must be called with lifecycleMutex held */
void updateProviders() {
    mutex_lock(&dataMutex.handle);
    const bool hasSubscribers = subscriptions != nullptr;
    mutex_unlock(&dataMutex.handle);

    const bool shouldBeActive = running && hasSubscribers;
    if (shouldBeActive == providersActive) {
        return;
    }
    providersActive = shouldBeActive;
    for (auto& host : hosts) {
        if (host.api == nullptr) {
            continue;
        }
        if (shouldBeActive) {
            startProvider(&host);
        } else {
            // Fixes are kept as the last known location
            stopProvider(&host);
        }
    }
}

void* createService(const ServiceManifest*) {
    return nullptr;
}

void destroyService(const ServiceManifest*, void*) {}

error_t onStart(ServiceInstance*, void*) {
    mutex_lock(&lifecycleMutex.handle);
    running = true;
    updateProviders();
    mutex_unlock(&lifecycleMutex.handle);
    return ERROR_NONE;
}

void onStop(ServiceInstance*, void*) {
    mutex_lock(&lifecycleMutex.handle);
    running = false;
    updateProviders();
    for (auto& host : hosts) {
        clearLocation(&host);
    }
    mutex_unlock(&lifecycleMutex.handle);
}

} // namespace

extern "C" {

error_t location_set_enabled(bool enabled) {
    mutex_lock(&settingsMutex.handle);
    ensureSettingsLoaded();
    settings.enabled = enabled;
    const LocationSettings copy = settings;
    mutex_unlock(&settingsMutex.handle);

    const error_t saved = location_settings_save(&copy);
    error_t result;
    if (enabled) {
        result = service_manager_start(LOCATION_SERVICE_ID);
        if (result == ERROR_INVALID_STATE) {
            result = ERROR_NONE;
        }
    } else {
        result = service_manager_stop(LOCATION_SERVICE_ID);
        if (result == ERROR_NOT_FOUND) {
            result = ERROR_NONE;
        }
    }
    return result != ERROR_NONE ? result : saved;
}

bool location_is_enabled(void) {
    mutex_lock(&settingsMutex.handle);
    ensureSettingsLoaded();
    const bool enabled = settings.enabled;
    mutex_unlock(&settingsMutex.handle);
    return enabled;
}

error_t location_set_precision(LocationPrecision precision) {
    mutex_lock(&settingsMutex.handle);
    ensureSettingsLoaded();
    settings.precision = precision;
    const LocationSettings copy = settings;
    mutex_unlock(&settingsMutex.handle);

    if (precision == LOCATION_PRECISION_APPROXIMATE) {
        // Fixes reported with precise precision are dropped until their providers report again
        mutex_lock(&dataMutex.handle);
        for (auto& host : hosts) {
            if (host.hasLocation && !(host.location.horizontal_accuracy >= LOCATION_APPROXIMATE_ACCURACY_METERS)) {
                host.hasLocation = false;
            }
        }
        mutex_unlock(&dataMutex.handle);
    }
    return location_settings_save(&copy);
}

LocationPrecision location_get_precision(void) {
    mutex_lock(&settingsMutex.handle);
    ensureSettingsLoaded();
    const LocationPrecision precision = settings.precision;
    mutex_unlock(&settingsMutex.handle);
    return precision;
}

void location_make_approximate(Location* location) {
    location->latitude = std::round(location->latitude / APPROXIMATE_GRID_DEGREES) * APPROXIMATE_GRID_DEGREES;
    location->longitude = std::round(location->longitude / APPROXIMATE_GRID_DEGREES) * APPROXIMATE_GRID_DEGREES;
    if (!(location->horizontal_accuracy >= LOCATION_APPROXIMATE_ACCURACY_METERS)) {
        location->horizontal_accuracy = LOCATION_APPROXIMATE_ACCURACY_METERS;
    }
    location->altitude = NAN;
    location->ellipsoidal_altitude = NAN;
    location->vertical_accuracy = NAN;
    location->speed = NAN;
    location->speed_accuracy = NAN;
    location->course = NAN;
    location->course_accuracy = NAN;
    location->satellites = 0;
}

error_t location_provider_register(const LocationProviderApi* api) {
    if (strlen(api->id) > LOCATION_PROVIDER_ID_MAX_LENGTH) {
        return ERROR_INVALID_ARGUMENT;
    }

    mutex_lock(&lifecycleMutex.handle);
    LocationHost* freeHost = nullptr;
    for (auto& host : hosts) {
        if (host.api == nullptr) {
            if (freeHost == nullptr) {
                freeHost = &host;
            }
        } else if (strcmp(host.api->id, api->id) == 0) {
            mutex_unlock(&lifecycleMutex.handle);
            return ERROR_ALREADY_EXISTS;
        }
    }
    if (freeHost == nullptr) {
        mutex_unlock(&lifecycleMutex.handle);
        return ERROR_RESOURCE;
    }
    freeHost->api = api;
    mutex_lock(&dataMutex.handle);
    freeHost->fallback = api->fallback;
    mutex_unlock(&dataMutex.handle);
    if (providersActive) {
        startProvider(freeHost);
    }
    mutex_unlock(&lifecycleMutex.handle);
    return ERROR_NONE;
}

error_t location_provider_unregister(const LocationProviderApi* api) {
    error_t result = ERROR_NOT_FOUND;
    mutex_lock(&lifecycleMutex.handle);
    for (auto& host : hosts) {
        if (host.api == api) {
            stopProvider(&host);
            clearLocation(&host);
            host.api = nullptr;
            result = ERROR_NONE;
            break;
        }
    }
    mutex_unlock(&lifecycleMutex.handle);
    return result;
}

void location_host_report(LocationHost* host, const Location* location) {
    mutex_lock(&dataMutex.handle);
    host->location = *location;
    strcpy(host->location.provider, host->api->id);
    host->location.uptime_ms = getUptimeMs();
    host->hasLocation = true;
    if (findBest() == host) {
        for (auto* sub = subscriptions; sub != nullptr; sub = sub->internal.next) {
            task_event_group_signal(sub->internal.event_group, sub->bit);
        }
    }
    mutex_unlock(&dataMutex.handle);
}

bool location_host_needs_fallback(void) {
    mutex_lock(&dataMutex.handle);
    const uint64_t now = getUptimeMs();
    bool needed = true;
    for (const auto& host : hosts) {
        if (host.hasLocation && getRank(host, now) == 2) {
            needed = false;
            break;
        }
    }
    mutex_unlock(&dataMutex.handle);
    return needed;
}

error_t location_get(Location* location) {
    mutex_lock(&dataMutex.handle);
    const auto* best = findBest();
    if (best != nullptr) {
        *location = best->location;
    }
    mutex_unlock(&dataMutex.handle);
    return best != nullptr ? ERROR_NONE : ERROR_NOT_FOUND;
}

error_t location_subscribe(LocationSubscription* sub, TaskEventGroup* event_group) {
    uint32_t bit;
    const error_t claimed = task_event_group_claim_bit(event_group, &bit);
    if (claimed != ERROR_NONE) {
        return claimed;
    }

    mutex_lock(&lifecycleMutex.handle);
    mutex_lock(&dataMutex.handle);
    for (auto* existing = subscriptions; existing != nullptr; existing = existing->internal.next) {
        if (existing == sub) {
            mutex_unlock(&dataMutex.handle);
            mutex_unlock(&lifecycleMutex.handle);
            task_event_group_release_bit(event_group, bit);
            return ERROR_INVALID_STATE;
        }
    }
    sub->bit = bit;
    sub->internal.event_group = event_group;
    sub->internal.next = subscriptions;
    subscriptions = sub;
    mutex_unlock(&dataMutex.handle);
    updateProviders();
    mutex_unlock(&lifecycleMutex.handle);
    return ERROR_NONE;
}

error_t location_unsubscribe(LocationSubscription* sub) {
    error_t result = ERROR_NOT_FOUND;
    mutex_lock(&lifecycleMutex.handle);
    mutex_lock(&dataMutex.handle);
    for (auto** link = &subscriptions; *link != nullptr; link = &(*link)->internal.next) {
        if (*link == sub) {
            *link = sub->internal.next;
            result = ERROR_NONE;
            break;
        }
    }
    mutex_unlock(&dataMutex.handle);
    updateProviders();
    mutex_unlock(&lifecycleMutex.handle);
    if (result == ERROR_NONE) {
        task_event_group_release_bit(sub->internal.event_group, sub->bit);
    }
    return result;
}

}

extern const ServiceManifest location_service_manifest = {
    .id = LOCATION_SERVICE_ID,
    .create_service = createService,
    .destroy_service = destroyService,
    .on_start = onStart,
    .on_stop = onStop,
};
