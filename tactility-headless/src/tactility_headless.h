#pragma once

#include "hardware_config.h"
#include "tactility_headless_config.h"

namespace tt {

void headless_init(const HardwareConfig* config);

const HardwareConfig* get_hardware_config();

} // namespace
