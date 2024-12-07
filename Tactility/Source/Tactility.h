#pragma once

#include "app/AppManifest.h"
#include "hal/Configuration.h"
#include "service/ServiceManifest.h"
#include "TactilityConfig.h"

namespace tt {

typedef struct {
    const hal::Configuration* hardware;
    // List of user applications
    const app::AppManifest* const apps[TT_CONFIG_APPS_LIMIT];
    const service::ServiceManifest* const services[TT_CONFIG_SERVICES_LIMIT];
    const char* autoStartAppId;
} Configuration;

/**
 * Attempts to initialize Tactility and all configured hardware.
 * @param config
 */
void run(const Configuration& config);

/**
 * While technically nullable, this instance is always set if tt_init() succeeds.
 * @return the Configuration instance that was passed on to tt_init() if init is successful
 */
const Configuration* _Nullable getConfiguration();

} // namespace
