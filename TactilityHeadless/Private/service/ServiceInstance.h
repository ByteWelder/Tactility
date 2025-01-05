#pragma once

#include "service/ServiceContext.h"

namespace tt::service {

class ServiceInstance : public ServiceContext {

private:

    Mutex mutex = Mutex(Mutex::TypeNormal);
    const service::ServiceManifest& manifest;
    std::shared_ptr<void> data = nullptr;

public:

    explicit ServiceInstance(const service::ServiceManifest& manifest);
    ~ServiceInstance() override = default;

    const service::ServiceManifest& getManifest() const override;
    std::shared_ptr<void> getData() const override;
    void setData(std::shared_ptr<void> newData) override;

    std::unique_ptr<Paths> getPaths() const override;
};

}
