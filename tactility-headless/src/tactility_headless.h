#pragma once

#include "hardware_config.h"
#include "tactility_headless_config.h"

void tt_headless_init(const HardwareConfig* config);

const HardwareConfig* tt_get_hardware_config();