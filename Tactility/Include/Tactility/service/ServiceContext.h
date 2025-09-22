#pragma once

#include <memory>

namespace tt::service {

struct ServiceManifest;
class ServicePaths;

/**
 * The public representation of a service instance.
 * @warning Do not store references or pointers to these! You can retrieve them via the Loader service.
 */
class ServiceContext {

protected:

    virtual ~ServiceContext() = default;

public:

    /** @return a reference to the service's manifest */
    virtual const ServiceManifest& getManifest() const = 0;

    /** Retrieve the paths that are relevant to this service */
    virtual std::unique_ptr<ServicePaths> getPaths() const = 0;
};


} // namespace
