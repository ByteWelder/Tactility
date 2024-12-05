#pragma once

#include "Mutex.h"
#include "ServiceManifest.h"
#include <memory>

namespace tt::service {

class ServiceContext {

protected:

    virtual ~ServiceContext() = default;

public:

    virtual const service::ServiceManifest& getManifest() const = 0;
    virtual std::shared_ptr<void> getData() const = 0;
    virtual void setData(std::shared_ptr<void> newData) = 0;
};

} // namespace
