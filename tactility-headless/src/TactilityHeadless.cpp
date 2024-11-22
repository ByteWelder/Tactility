#include "TactilityHeadless.h"
#include "hal/Configuration.h"
#include "hal/Hal_i.h"
#include "ServiceManifest.h"
#include "ServiceRegistry.h"

#ifdef ESP_PLATFORM
#include "EspInit.h"
#endif

namespace tt {

#define TAG "tactility"

namespace service::wifi { extern const ServiceManifest manifest; }
namespace service::sdcard { extern const ServiceManifest manifest; }

static const ServiceManifest* const system_services[] = {
    &service::sdcard::manifest,
    &service::wifi::manifest
};

static const hal::Configuration* hardwareConfig = nullptr;

static void register_and_start_system_services() {
    TT_LOG_I(TAG, "Registering and starting system services");
    int app_count = sizeof(system_services) / sizeof(ServiceManifest*);
    for (int i = 0; i < app_count; ++i) {
        service_registry_add(system_services[i]);
        tt_check(service_registry_start(system_services[i]->id));
    }
}

void headless_init(const hal::Configuration* config) {
#ifdef ESP_PLATFORM
    esp_init();
#endif
    hardwareConfig = config;
    hal::init(config);
    register_and_start_system_services();
}

const hal::Configuration* get_hardware_config() {
    return hardwareConfig;
}

} // namespace
