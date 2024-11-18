#include "service.h"
#include "service_manifest.h"

Service::Service(const ServiceManifest& manifest) : manifest(manifest) {}

const ServiceManifest& Service::getManifest() const { return manifest; }

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
