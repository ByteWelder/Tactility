#pragma once

#include "Language.h"
#include <string>

namespace tt::settings {

struct SystemSettings {
    Language language;
    bool timeFormat24h;
    std::string timeZoneCode;
};

bool loadSystemSettings(SystemSettings& properties);

bool saveSystemSettings(const SystemSettings& properties);

}
