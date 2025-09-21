#include <Tactility/service/ServiceInstance.h>

#include <Tactility/service/ServiceManifest.h>
#include <Tactility/service/ServicePaths.h>

namespace tt::service {

ServiceInstance::ServiceInstance(std::shared_ptr<const ServiceManifest> manifest) :
    manifest(manifest),
    service(manifest->createService())
{}

const ServiceManifest& ServiceInstance::getManifest() const { return *manifest; }

std::unique_ptr<ServicePaths> ServiceInstance::getPaths() const {
    return std::make_unique<ServicePaths>(manifest);
}

}