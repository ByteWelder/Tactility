#pragma once

#include <string>

namespace tt {

struct BootProperties {
    /** App to start automatically after the splash screen. */
    std::string launcherAppId;
    /** App to start automatically from the launcher screen. */
    std::string autoStartAppId;
};

/**
 * Load the boot properties from the relevant file location(s).
 * It will first attempt to load them from the SD card and if no file was found,
 * then it will try to load the one from the data mount point.
 *
 * @param[out] properties the resulting properties
 * @return true when the properties were successfully loaded and the result was set
 */
bool loadBootProperties(BootProperties& properties);

}
