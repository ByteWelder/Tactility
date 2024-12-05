#include "service/ServiceInstance.h"

namespace tt::service {

ServiceInstance::ServiceInstance(const service::ServiceManifest&manifest) : manifest(manifest) {}

const service::ServiceManifest& ServiceInstance::getManifest() const { return manifest; }

void* ServiceInstance::getData() const {
    mutex.acquire(TtWaitForever);
    void* data_copy = data;
    mutex.release();
    return data_copy;
}

void ServiceInstance::setData(void* newData) {
    mutex.acquire(TtWaitForever);
    data = newData;
    mutex.release();
}

}