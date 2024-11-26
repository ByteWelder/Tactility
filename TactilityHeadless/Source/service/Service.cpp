#include "Service.h"
#include "Manifest.h"

namespace tt::service {

Service::Service(const service::Manifest& manifest) : manifest(manifest) {}

const service::Manifest& Service::getManifest() const { return manifest; }

void* Service::getData() const {
    mutex.acquire(TtWaitForever);
    void* data_copy = data;
    mutex.release();
    return data_copy;
}

void Service::setData(void* newData) {
    mutex.acquire(TtWaitForever);
    data = newData;
    mutex.release();
}

} // namespace
