#pragma once

#include "Language.h"

namespace tt::settings {

struct SettingsProperties {
    Language language;
    bool timeFormat24h;
};

bool loadSettingsProperties(SettingsProperties& properties);

bool saveSettingsProperties(const SettingsProperties& properties);

}
