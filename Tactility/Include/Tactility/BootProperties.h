#pragma once

#include <string>

namespace tt {

struct BootProperties {
    /** App to start automatically after the splash screen. */
    std::string launcherAppId;
    /** App to start automatically from the launcher screen. */
    std::string autoStartAppId;
};

bool loadBootProperties(BootProperties& properties);

bool saveBootProperties(const BootProperties& properties);

}
