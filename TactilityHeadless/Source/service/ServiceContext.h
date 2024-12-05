#pragma once

#include "Mutex.h"
#include "ServiceManifest.h"

namespace tt::service {

class ServiceContext {

protected:

    virtual ~ServiceContext() = default;

public:

    [[nodiscard]] virtual const service::ServiceManifest& getManifest() const = 0;
    [[nodiscard]] virtual void* getData() const = 0;
    virtual void setData(void* newData) = 0;
};

} // namespace
