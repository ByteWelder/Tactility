#pragma once

#include "hardware_config.h"
#include "service_manifest.h"
#include "tactility_headless_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void tt_tactility_headless_init(const HardwareConfig* config, const ServiceManifest* const services[32]);

const HardwareConfig* tt_get_hardware_config();

#ifdef __cplusplus
}
#endif