#include <ctime>
#include "Time.h"
#include "Preferences.h"
#include "kernel/SystemEvents.h"

static std::string timeZoneName;
static std::string timeZoneCode;

namespace tt::time {

#ifdef ESP_PLATFORM

#define TIME_SETTINGS_NAMESPACE "time"

#define TIMEZONE_PREFERENCES_KEY_NAME "tz_name"
#define TIMEZONE_PREFERENCES_KEY_CODE "tz_code"

void init() {
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

    kernel::systemEventPublish(kernel::SystemEvent::Time);
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

void init() {}

void setTimeZone(const std::string& name, const std::string& code) {
    timeZoneName = name;
    timeZoneCode = code;
    kernel::systemEventPublish(kernel::SystemEvent::Time);
}

std::string getTimeZoneName() {
    return timeZoneName;
}

std::string getTimeZoneCode() {
    return timeZoneCode;
}

#endif

}
