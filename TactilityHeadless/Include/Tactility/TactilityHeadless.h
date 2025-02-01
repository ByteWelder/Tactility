#pragma once

#include "Tactility/hal/Configuration.h"

#include <Tactility/TactilityCore.h>
#include <Tactility/Dispatcher.h>

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

/** While technically this configuration is nullable, it's never null after initHeadless() is called. */
const Configuration* _Nullable getConfiguration();

} // namespace
