#pragma once

#include "hal/Configuration.h"
#include "TactilityHeadlessConfig.h"

namespace tt {

void initHeadless(const hal::Configuration& config);

} // namespace

namespace tt::hal {

const Configuration& getConfiguration();

} // namespace
