#pragma once

#include "Tactility/app/AppManifest.h"

#include <Tactility/hal/Configuration.h>
#include <Tactility/service/ServiceManifest.h>

namespace tt {

namespace app::launcher { extern const app::AppManifest manifest; }

/** @brief The configuration for the operating system
 * It contains the hardware configuration, apps and services
 */
struct Configuration {
    /** HAL configuration (drivers) */
    const hal::Configuration* hardware = nullptr;
    /** List of user applications */
    const std::vector<const app::AppManifest*> apps = {};
    /** List of user services */
    const std::vector<const service::ServiceManifest*> services = {};
    /** Optional app to start automatically after the splash screen. */
    const std::string launcherAppId = app::launcher::manifest.id;
    /** Optional app to start automatically after the splash screen. */
    const std::string autoStartAppId = {};
};

/**
 * Attempts to initialize Tactility and all configured hardware.
 * @param[in] config
 */
void run(const Configuration& config);

/**
 * While technically nullable, this instance is always set if tt_init() succeeds.
 * @return the Configuration instance that was passed on to tt_init() if init is successful
 */
const Configuration* _Nullable getConfiguration();

} // namespace
