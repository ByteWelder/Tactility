#pragma once

#include <service/instance.h>

#include <TactilityCpp/Allocator.h>
#include <TactilityCpp/Memory.h>

#include <memory>

namespace tt::service {

using State = ::ServiceState;

// Forward declaration
class ServiceContext;

class Service {

public:

    Service() = default;
    virtual ~Service() = default;

    virtual bool onStart(ServiceContext& serviceContext) { return true; }
    virtual void onStop(ServiceContext& serviceContext) {}
};

template<typename T>
void* create(const ::ServiceManifest* /*manifest*/) {
    return newObject<::std::shared_ptr<Service>>(::std::allocate_shared<T>(OptExternalAllocator<T>()));
}

}