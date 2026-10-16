#pragma once

#include <tactility/drivers/radio.h>

#include <string>

namespace tt::settings::radio {

/** @return the persisted regulatory region, RADIO_REGION_UNSET when none was chosen */
RadioRegion loadRegion();

/** Persist the regulatory region and apply it to all radio devices */
bool saveRegion(RadioRegion region);

/** Apply the persisted region to all radio devices */
void applyRegion();

/** @return a human-readable name, e.g. "Europe 868 MHz" */
const char* getRegionLabel(RadioRegion region);

/** @return true when the devicetree has a radio, whether it's started or not */
bool hasRadio();

/**
 * @param[in] timeZoneName an IANA time zone name, e.g. "Europe/Amsterdam"
 * @return the region of the most common radio band where that time zone is used, RADIO_REGION_UNSET when unknown
 */
RadioRegion findRegionForTimeZone(const std::string& timeZoneName);

}
