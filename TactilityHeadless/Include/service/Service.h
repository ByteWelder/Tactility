#pragma once

#include <memory>

namespace tt::service {

// Forward declaration
class ServiceContext;

class Service {

public:

    Service() = default;
    virtual ~Service() = default;

    virtual void onStart(ServiceContext& serviceContext) {}
    virtual void onStop(ServiceContext& serviceContext) {}
};

template<typename T>
std::shared_ptr<Service> create() { return std::shared_ptr<T>(new T); }

}