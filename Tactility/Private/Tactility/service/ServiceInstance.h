#pragma once

#include <Tactility/service/ServiceContext.h>
#include <Tactility/service/Service.h>
#include <Tactility/Mutex.h>

#include <memory>

namespace tt::service {

class ServiceInstance final : public ServiceContext {

    Mutex mutex = Mutex(Mutex::Type::Normal);
    std::shared_ptr<const ServiceManifest> manifest;
    std::shared_ptr<Service> service;
    State state = State::Stopped;

public:

    explicit ServiceInstance(std::shared_ptr<const ServiceManifest> manifest);
    ~ServiceInstance() override = default;

    /** @return a reference to the service's manifest */
    const ServiceManifest& getManifest() const override;

    /** Retrieve the paths that are relevant to this service */
    std::unique_ptr<ServicePaths> getPaths() const override;

    std::shared_ptr<Service> getService() const { return service; }

    State getState() const { return state; }

    void setState(State newState) { state = newState; }
};

}
