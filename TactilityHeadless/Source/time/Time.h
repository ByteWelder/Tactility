#pragma once

#include <string>

namespace tt::time {

void setTimeZone(const std::string& name, const std::string& code);
std::string getTimeZoneName();
std::string getTimeZoneCode();

}
