#include "tactility_headless.h"
#include "hardware_config.h"
#include "hardware_i.h"
#include "service_registry.h"

static const HardwareConfig* hardwareConfig = NULL;

void tt_tactility_headless_init(const HardwareConfig* config, const ServiceManifest* const services[32]) {
    tt_service_registry_init();
    tt_hardware_init(config);
    hardwareConfig = config;
}

const HardwareConfig* tt_get_hardware_config() {
    return hardwareConfig;
}
