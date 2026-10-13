// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <location/location.h>

#define LOCATION_SERVICE_ID "tactility.location"

struct LocationSettings {
    bool enabled;
    enum LocationPrecision precision;
};

/** Loads the settings, or defaults when there are none */
void location_settings_load(struct LocationSettings* settings);

error_t location_settings_save(const struct LocationSettings* settings);
