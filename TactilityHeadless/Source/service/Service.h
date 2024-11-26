#pragma once

#include "Mutex.h"
#include "Manifest.h"

namespace tt::service {

class Service {
private:
    Mutex mutex = Mutex(MutexTypeNormal);
    const service::Manifest& manifest;
    void* data = nullptr;

public:
    Service(const service::Manifest& manifest);

    [[nodiscard]] const service::Manifest& getManifest() const;
    [[nodiscard]] void* getData() const;
    void setData(void* newData);
};

} // namespace
