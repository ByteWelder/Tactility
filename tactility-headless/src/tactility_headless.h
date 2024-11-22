#pragma once

#include "hal/Configuration.h"
#include "tactility_headless_config.h"

namespace tt {

void headless_init(const hal::Configuration* config);

const hal::Configuration* get_hardware_config();

} // namespace
