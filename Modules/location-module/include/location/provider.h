// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <location/location.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Interface between the location service and a location provider. Not part of the app SDK.
 */

/** The service, as seen by a provider */
struct LocationHost;

struct LocationProviderApi {
    /** Unique id, at most LOCATION_PROVIDER_ID_MAX_LENGTH characters */
    const char* id;

    /**
     * A fallback provider (e.g. IP geolocation) is only used while no other provider reported a fix in the last
     * 2 minutes. It should only work when location_host_needs_fallback() returns true.
     */
    bool fallback;

    /**
     * Start the provider. Called when the service starts, or when the provider registers while the service runs.
     * @return the provider state, NULL on failure
     */
    void* (*create)(struct LocationHost* host);

    /** Stop the provider. It must not report anymore once this returns. */
    void (*destroy)(void* provider);
};

/** Maximum number of registered providers */
#define LOCATION_PROVIDER_MAX_COUNT 4

/**
 * Register a provider. Usually called from the provider module's start().
 * @retval ERROR_ALREADY_EXISTS when a provider with the same id is registered
 * @retval ERROR_RESOURCE when LOCATION_PROVIDER_MAX_COUNT providers are registered
 */
error_t location_provider_register(const struct LocationProviderApi* api);

/** @retval ERROR_NOT_FOUND when @a api is not registered */
error_t location_provider_unregister(const struct LocationProviderApi* api);

/**
 * Report a new fix. Can be called from any task.
 * The service sets the `provider` and `uptime_ms` fields.
 */
void location_host_report(struct LocationHost* host, const struct Location* location);

/**
 * Reduce a fix to LOCATION_PRECISION_APPROXIMATE: the position snaps to a grid and only position and time remain.
 * Providers that can report precise fixes apply this when location_get_precision() is LOCATION_PRECISION_APPROXIMATE.
 */
void location_make_approximate(struct Location* location);

/** @return true when no provider that isn't a fallback reported a fix in the last 2 minutes */
bool location_host_needs_fallback(void);

#ifdef __cplusplus
}
#endif
