#pragma once

#include "Hal/Configuration.h"
#include "TactilityHeadlessConfig.h"

namespace tt {

void init(const hal::Configuration& config);

const hal::Configuration& get_hardware_config();

} // namespace
