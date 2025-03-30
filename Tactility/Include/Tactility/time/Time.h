#pragma once

#include <string>

namespace tt::time {

/**
 * Set the timezone
 * @param[in] name human-readable name
 * @param[in] code the technical code (from timezones.csv)
 */
void setTimeZone(const std::string& name, const std::string& code);

/**
 * Get the name of the timezone
 */
std::string getTimeZoneName();

/**
 * Get the code of the timezone (see timezones.csv)
 */
std::string getTimeZoneCode();

/** @return true when clocks should be shown as a 24 hours one instead of 12 hours */
bool isTimeFormat24Hour();

/** Set whether clocks should be shown as a 24 hours instead of 12 hours
 * @param[in] show24Hour
 */
void setTimeFormat24Hour(bool show24Hour);

}
