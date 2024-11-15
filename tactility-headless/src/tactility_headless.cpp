#include "tactility_headless.h"
#include "hardware_config.h"
#include "hardware_i.h"
#include "service_manifest.h"
#include "service_registry.h"

#ifdef ESP_PLATFORM
#include "esp_init.h"
#endif

#define TAG "tactility"

extern ServiceManifest sdcard_service;
extern ServiceManifest wifi_service;

static const ServiceManifest* const system_services[] = {
    &sdcard_service,
    &wifi_service
};

static const HardwareConfig* hardwareConfig = nullptr;

static void register_and_start_system_services() {
    TT_LOG_I(TAG, "Registering and starting system services");
    int app_count = sizeof(system_services) / sizeof(ServiceManifest*);
    for (int i = 0; i < app_count; ++i) {
        tt_service_registry_add(system_services[i]);
        tt_check(tt_service_registry_start(system_services[i]->id));
    }
}

void tt_headless_init(const HardwareConfig* config) {
#ifdef ESP_PLATFORM
    tt_esp_init();
#endif
    hardwareConfig = config;
    tt_service_registry_init();
    tt_hardware_init(config);
    register_and_start_system_services();
}

const HardwareConfig* tt_get_hardware_config() {
    return hardwareConfig;
}
