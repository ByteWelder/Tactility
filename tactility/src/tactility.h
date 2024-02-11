#pragma once

#include "app_manifest.h"
#include "hardware_config.h"
#include "service_manifest.h"
#include "tactility_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const HardwareConfig* hardware;
    // List of user applications
    const AppManifest* const apps[TT_CONFIG_APPS_LIMIT];
    const ServiceManifest* const services[TT_CONFIG_SERVICES_LIMIT];
    const char* auto_start_app_id;
} Config;

/**
 * Attempts to initialize Tactility and all configured hardware.
 * @param config
 */
void tt_init(const Config* config);

/**
 * While technically nullable, this instance is always set if tt_init() succeeds.
 * @return the Configuration instance that was passed on to tt_init() if init is successful
 */
const Config* _Nullable tt_get_config();

#ifdef __cplusplus
}
#endif
