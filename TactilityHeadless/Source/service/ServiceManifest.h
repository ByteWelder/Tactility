#pragma once

#include <string>

namespace tt::service {

class ServiceContext;

typedef void (*ServiceOnStart)(ServiceContext& service);
typedef void (*ServiceOnStop)(ServiceContext& service);

struct ServiceManifest {
    /**
     * The identifier by which the app is launched by the system and other apps.
     */
    std::string id {};

    /**
     * Non-blocking method to call when service is started.
     */
    const ServiceOnStart onStart = nullptr;

    /**
     * Non-blocking method to call when service is stopped.
     */
    const ServiceOnStop onStop = nullptr;

};

} // namespace
