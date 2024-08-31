#pragma once

#include "hardware_config.h"
#include "tactility_headless_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void tt_headless_init(const HardwareConfig* config);

const HardwareConfig* tt_get_hardware_config();

#ifdef __cplusplus
}
#endif