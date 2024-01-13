#pragma once

#include "app_manifest.h"
#include "devices.h"
#include "furi_extra_defines.h"
#include "service_manifest.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_APPS_LIMIT 32
#define CONFIG_SERVICES_LIMIT 32

// Forward declarations
typedef void* FuriThreadId;
typedef void (*Bootstrap)();
typedef TouchDriver (*CreateTouchDriver)();
typedef DisplayDriver (*CreateDisplayDriver)();

typedef struct {
    // Optional bootstrapping method
    const Bootstrap _Nullable bootstrap;
    // Required driver for display
    const CreateDisplayDriver _Nonnull display_driver;
    // Optional driver for touch input
    const CreateTouchDriver _Nullable touch_driver;
} HardwareConfig;

typedef struct {
    const HardwareConfig* hardware;
    // List of user applications
    const AppManifest* const apps[CONFIG_APPS_LIMIT];
    const ServiceManifest* const services[CONFIG_SERVICES_LIMIT];
    const char* auto_start_app_id;
} Config;

__attribute__((unused)) extern void tactility_start(const Config _Nonnull* config);

#ifdef __cplusplus
}
#endif
