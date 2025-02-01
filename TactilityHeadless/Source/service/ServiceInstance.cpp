#include "Tactility/service/ServiceInstance.h"
#include "Tactility/service/ServiceInstancePaths.h"

namespace tt::service {

ServiceInstance::ServiceInstance(std::shared_ptr<const service::ServiceManifest> manifest) :
    manifest(manifest),
    service(manifest->createService())
{}

const service::ServiceManifest& ServiceInstance::getManifest() const { return *manifest; }

std::unique_ptr<Paths> ServiceInstance::getPaths() const {
    return std::make_unique<ServiceInstancePaths>(manifest);
}

}