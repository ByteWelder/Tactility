#pragma once

#include "Tactility/service/ServiceContext.h"
#include "Tactility/service/Service.h"

namespace tt::service {

class ServiceInstance : public ServiceContext {

private:

    Mutex mutex = Mutex(Mutex::Type::Normal);
    std::shared_ptr<const ServiceManifest> manifest;
    std::shared_ptr<Service> service;

public:

    explicit ServiceInstance(std::shared_ptr<const service::ServiceManifest> manifest);
    ~ServiceInstance() override = default;

    /** @return a reference ot the service's manifest */
    const service::ServiceManifest& getManifest() const override;

    /** Retrieve the paths that are relevant to this service */
    std::unique_ptr<Paths> getPaths() const override;

    std::shared_ptr<Service> getService() const { return service; }
};

}
