#pragma once

#include "hal/Configuration.h"
#include "TactilityHeadlessConfig.h"

namespace tt {

void headless_init(const hal::Configuration* config);

const hal::Configuration* get_hardware_config();

} // namespace
