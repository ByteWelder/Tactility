#pragma once

namespace tt {

/** Queues a function on the main dispatcher's task without using the deprecated MainDispatcher class.
 * @return true if the function was queued */
bool dispatchOnMainDispatcher(void (*function)(void* context), void* context);

void prepareFileSystems();

/** Starts the kernel modules that Tactility depends on. */
void initModules();

/** Registers and starts the primary system services. */
void registerAndStartServices();

/** Registers the internal apps and the apps installed on mounted file systems. */
void registerApps();

/** Registers and starts the Boot app. */
void startBootApp();

}
