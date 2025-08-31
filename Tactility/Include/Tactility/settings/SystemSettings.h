#pragma once

#include "Language.h"

namespace tt::settings {

struct SystemSettings {
    Language language;
    bool timeFormat24h;
};

bool loadSystemSettings(SystemSettings& properties);

bool saveSystemSettings(const SystemSettings& properties);

}
