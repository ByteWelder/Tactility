// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <location/provider.h>

#include <stddef.h>

/**
 * Fallback provider that looks up the location of the public IP address with json.geoiplookup.io.
 * The result is reused while the network (local IP address and gateway) is unchanged.
 */
extern const LocationProviderApi ip_location_provider;

/** Start tracking the network that the device is connected to, also while the provider doesn't run */
error_t ip_provider_track_network(void);

/** Stop tracking the network and release the cached result. The provider must not run. */
void ip_provider_untrack_network(void);

struct IpLookupResponse {
    bool success;
    double latitude;
    double longitude;
    /** IANA time zone name, empty when unknown */
    char time_zone[LOCATION_TIME_ZONE_MAX_LENGTH + 1];
};

/**
 * Parse the JSON response of json.geoiplookup.io
 * @param[in] response the NUL-terminated response body
 * @return false when the response is malformed
 */
bool ip_lookup_parse_response(const char* response, IpLookupResponse* out);
