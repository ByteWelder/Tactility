#pragma once

#include "app/AppManifest.h"
#include "hal/Configuration.h"
#include "service/ServiceManifest.h"
#include "TactilityConfig.h"

namespace tt {

/** @brief The configuration for the operating system
 * It contains the hardware configuration, apps and services
 */
struct Configuration {
    /** HAL configuration (drivers) */
    const hal::Configuration* hardware;
    /** List of user applications */
    const app::AppManifest* const apps[TT_CONFIG_APPS_LIMIT] = {};
    /** List of user services */
    const service::ServiceManifest* const services[TT_CONFIG_SERVICES_LIMIT] = {};
    /** Optional app to start automatically after the splash screen. */
    const char* _Nullable autoStartAppId = nullptr;
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
