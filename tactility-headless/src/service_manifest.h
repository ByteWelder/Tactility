#pragma once

#include "tactility_core.h"

typedef void* Service;

typedef void (*ServiceOnStart)(Service service);
typedef void (*ServiceOnStop)(Service service);

typedef struct {
    /**
     * The identifier by which the app is launched by the system and other apps.
     */
    const char* id;

    /**
     * Non-blocking method to call when service is started.
     */
    const ServiceOnStart _Nullable on_start;

    /**
     * Non-blocking method to call when service is stopped.
     */
    const ServiceOnStop _Nullable on_stop;

} ServiceManifest;
