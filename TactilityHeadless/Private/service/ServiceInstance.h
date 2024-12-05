#pragma once

#include "service/ServiceContext.h"

namespace tt::service {

class ServiceInstance : public ServiceContext {

private:

    Mutex mutex = Mutex(MutexTypeNormal);
    const service::ServiceManifest& manifest;
    void* data = nullptr;

public:

    explicit ServiceInstance(const service::ServiceManifest& manifest);
    ~ServiceInstance() override = default;

    [[nodiscard]] const service::ServiceManifest& getManifest() const override;
    [[nodiscard]] void* getData() const override;
    void setData(void* newData) override;
};

}
