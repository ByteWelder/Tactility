// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <location/location.h>
#include <location/provider.h>

#include <minmea.h>

/** Uses the first started GPS device */
extern const LocationProviderApi gps_location_provider;

/**
 * Convert the sentences of one GPS epoch into a fix.
 * @param[in] gga the GGA sentence of the same epoch, NULL when there is none
 * @return false when @a rmc has no valid fix
 */
bool gps_location_from_nmea(const minmea_sentence_rmc* rmc, const minmea_sentence_gga* gga, Location* location);
