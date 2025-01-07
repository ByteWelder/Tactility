#pragma once

#include "TactilityCore.h"
#include "hal/Configuration.h"
#include "TactilityHeadlessConfig.h"
#include "Dispatcher.h"

namespace tt {

/** Initialize the hardware and started the internal services. */
void initHeadless(const hal::Configuration& config);

/** Provides access to the dispatcher that runs on the main task.
 * @warning This dispatcher is used for WiFi and might block for some time during WiFi connection.
 * @return the dispatcher
 */
Dispatcher& getMainDispatcher();

} // namespace

namespace tt::hal {

/** Can be called after initHeadless() is called. Will crash otherwise. */
const Configuration& getConfiguration();

} // namespace
