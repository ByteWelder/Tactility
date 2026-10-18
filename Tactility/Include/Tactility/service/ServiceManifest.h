#pragma once

#include <Tactility/service/Service.h>

#include <string>

namespace tt::service {

// Forward declarations
class ServiceContext;

using CreateService = ::ServiceCreate;

/** A ledger that describes the main parts of a service. */
struct ServiceManifest {
    /** The identifier by which the app is launched by the system and other apps. */
    std::string id {};

    /**
     * Creates the instance of the service: use create<T>(), or return a std::shared_ptr<Service> that is allocated with tt::newObject().
     * The registration releases the returned pointer with tt::deleteObject() when the service is removed,
     * so a pointer from plain new is released incorrectly.
     */
    CreateService createService = nullptr;
};

} // namespace
