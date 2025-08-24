#include "Tactility/settings/Time.h"
#include "Tactility/kernel/SystemEvents.h"

#include <Tactility/settings/SettingsProperties.h>

#ifdef ESP_PLATFORM
#include <ctime>
#include "Tactility/Preferences.h"
#endif

namespace tt::settings {

#ifdef ESP_PLATFORM

#define TIME_SETTINGS_NAMESPACE "time"

#define TIMEZONE_PREFERENCES_KEY_NAME "tz_name"
#define TIMEZONE_PREFERENCES_KEY_CODE "tz_code"
#define TIMEZONE_PREFERENCES_KEY_TIME24 "tz_time24"

void initTimeZone() {
    auto code= getTimeZoneCode();
    if (!code.empty()) {
        setenv("TZ", code.c_str(), 1);
        tzset();
    }
}

void setTimeZone(const std::string& name, const std::string& code) {
    Preferences preferences(TIME_SETTINGS_NAMESPACE);
    preferences.putString(TIMEZONE_PREFERENCES_KEY_NAME, name);
    preferences.putString(TIMEZONE_PREFERENCES_KEY_CODE, code);

    setenv("TZ", code.c_str(), 1);
    tzset();

    kernel::publishSystemEvent(kernel::SystemEvent::Time);
}

std::string getTimeZoneName() {
    Preferences preferences(TIME_SETTINGS_NAMESPACE);
    std::string result;
    if (preferences.optString(TIMEZONE_PREFERENCES_KEY_NAME, result)) {
        return result;
    } else {
        return {};
    }
}

std::string getTimeZoneCode() {
    Preferences preferences(TIME_SETTINGS_NAMESPACE);
    std::string result;
    if (preferences.optString(TIMEZONE_PREFERENCES_KEY_CODE, result)) {
        return result;
    } else {
        return {};
    }
}

#else

static std::string timeZoneName;
static std::string timeZoneCode;
static bool show24Hour = true;

void init() {}

void setTimeZone(const std::string& name, const std::string& code) {
    timeZoneName = name;
    timeZoneCode = code;
    kernel::publishSystemEvent(kernel::SystemEvent::Time);
}

std::string getTimeZoneName() {
    return timeZoneName;
}

std::string getTimeZoneCode() {
    return timeZoneCode;
}

#endif

bool isTimeFormat24Hour() {
    SettingsProperties properties;
    if (!loadSettingsProperties(properties)) {
        return true;
    } else {
        return properties.timeFormat24h;
    }
}

void setTimeFormat24Hour(bool show24Hour) {
    SettingsProperties properties;
    if (!loadSettingsProperties(properties)) {
        return;
    }

    properties.timeFormat24h = show24Hour;
    saveSettingsProperties(properties);
}

}
