#pragma once

#include "Mutex.h"
#include "ServiceManifest.h"

namespace tt::service {

class ServiceContext {

protected:

    virtual ~ServiceContext() = default;

public:

    virtual const service::ServiceManifest& getManifest() const = 0;
    virtual void* getData() const = 0;
    virtual void setData(void* newData) = 0;
};

} // namespace
