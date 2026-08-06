#include <Tactility/service/ServiceContext.h>

#include <Tactility/service/ServicePaths.h>
#include <service/instance.h>

namespace tt::service {

const ::ServiceManifest* ServiceContext::getManifest() const {
    return service_instance_get_manifest(service_instance);
}

std::unique_ptr<ServicePaths> ServiceContext::getPaths() const {
    return std::make_unique<ServicePaths>(getManifest());
}

} // namespace
