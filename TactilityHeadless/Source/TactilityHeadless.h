#pragma once

#include "Hal/Configuration.h"
#include "TactilityHeadlessConfig.h"

namespace tt {

void initHeadless(const hal::Configuration& config);

} // namespace

namespace tt::hal {

const Configuration& getConfiguration();

} // namespace
