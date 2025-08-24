#include "tt_time.h"

#include <Tactility/settings/Time.h>
#include <cstring>

using namespace tt;

extern "C" {

void tt_timezone_set(const char* name, const char* code) {
    settings::setTimeZone(name, code);
}

bool tt_timezone_get_name(char* buffer, size_t bufferSize) {
    auto name = settings::getTimeZoneName();
    if (bufferSize < (name.length() + 1)) {
        return false;
    } else {
        strcpy(buffer, name.c_str());
        return true;
    }
}

bool tt_timezone_get_code(char* buffer, size_t bufferSize) {
    auto code = settings::getTimeZoneCode();
    if (bufferSize < (code.length() + 1)) {
        return false;
    } else {
        strcpy(buffer, code.c_str());
        return true;
    }
}

bool tt_timezone_is_format_24_hour() {
    return settings::isTimeFormat24Hour();
}

void tt_timezone_set_format_24_hour(bool show24Hour) {
    return settings::setTimeFormat24Hour(show24Hour);
}

}
