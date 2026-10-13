// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <location/provider.h>

#include <stddef.h>

/**
 * Fallback provider that looks up the location of the public IP address with ip-api.com.
 * The result is reused while the network (local IP address and gateway) is unchanged.
 */
extern const LocationProviderApi ip_location_provider;

/** Start tracking the network that the device is connected to, also while the provider doesn't run */
error_t ip_provider_track_network(void);

/** Stop tracking the network and release the cached result. The provider must not run. */
void ip_provider_untrack_network(void);

#define IP_API_MESSAGE_MAX_LENGTH 63

struct IpApiResponse {
    /** HTTP status code */
    int status_code;
    /** Requests left in the rate limit window, -1 when unknown */
    int rate_remaining;
    /** Seconds until the rate limit window resets, -1 when unknown */
    int rate_reset_seconds;
    bool success;
    /** Reason of failure, empty on success */
    char message[IP_API_MESSAGE_MAX_LENGTH + 1];
    double latitude;
    double longitude;
};

/**
 * Parse an HTTP response of the newline separated ip-api endpoint, requested with fields "status,message,lat,lon"
 * @param[in] response the NUL-terminated response, including the status line and headers
 * @return false when the response is malformed
 */
bool ip_api_parse_response(const char* response, IpApiResponse* out);
