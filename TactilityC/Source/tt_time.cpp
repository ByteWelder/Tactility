#include "tt_time.h"

#include <Tactility/time/Time.h>

using namespace tt;

extern "C" {

void tt_timezone_set(const char* name, const char* code) {
    time::setTimeZone(name, code);
}

const char* tt_timezone_get_name() {
    return time::getTimeZoneName().c_str();
}

const char* tt_timezone_get_code() {
    return time::getTimeZoneCode().c_str();
}

bool tt_timezone_is_format_24_hour() {
    return time::isTimeFormat24Hour();
}

void tt_timezone_set_format_24_hour(bool show24Hour) {
    return time::setTimeFormat24Hour(show24Hour);
}

}
