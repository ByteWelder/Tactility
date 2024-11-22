#pragma once

#include "Mutex.h"
#include "service_manifest.h"

namespace tt {

class Service {
private:
    Mutex mutex = Mutex(MutexTypeNormal);
    const ServiceManifest& manifest;
    void* data = nullptr;

public:
    Service(const ServiceManifest& manifest);

    [[nodiscard]] const ServiceManifest& getManifest() const;
    [[nodiscard]] void* getData() const;
    void setData(void* newData);
};

} // namespace
