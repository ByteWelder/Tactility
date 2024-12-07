#pragma once

#include "TactilityCore.h"
#include "hal/Configuration.h"
#include "TactilityHeadlessConfig.h"
#include "Dispatcher.h"

namespace tt {

void initHeadless(const hal::Configuration& config);

Dispatcher& getMainDispatcher();

} // namespace

namespace tt::hal {

const Configuration& getConfiguration();

} // namespace
