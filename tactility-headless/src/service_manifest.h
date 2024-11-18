#pragma once

#include "tactility_core.h"
#include <string>

class Service;

typedef void (*ServiceOnStart)(Service& service);
typedef void (*ServiceOnStop)(Service& service);

typedef struct ServiceManifest {
    /**
     * The identifier by which the app is launched by the system and other apps.
     */
    std::string id {};

    /**
     * Non-blocking method to call when service is started.
     */
    const ServiceOnStart on_start = nullptr;

    /**
     * Non-blocking method to call when service is stopped.
     */
    const ServiceOnStop on_stop = nullptr;

} ServiceManifest;
