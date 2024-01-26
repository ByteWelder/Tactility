#pragma once

#include "app_manifest.h"
#include "hardware_config.h"
#include "service_manifest.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_APPS_LIMIT 32
#define CONFIG_SERVICES_LIMIT 32

typedef struct {
    const HardwareConfig* hardware;
    // List of user applications
    const AppManifest* const apps[CONFIG_APPS_LIMIT];
    const ServiceManifest* const services[CONFIG_SERVICES_LIMIT];
    const char* auto_start_app_id;
} Config;

TT_UNUSED void tt_init(const Config* config);

#ifdef __cplusplus
}
#endif
