#pragma once

#include "ServiceManifest.h"
#include "Service.h"

#include <memory>

namespace tt::service {

/** Register a service.
 * @param[in] the service manifest
 */
void addService(std::shared_ptr<const ServiceManifest> manifest, bool autoStart = true);

/** Register a service.
 * @param[in] the service manifest
 */
void addService(const ServiceManifest& manifest, bool autoStart = true);

/** Start a service.
 * @param[in] the service id as defined in its manifest
 * @return true on success
 */
bool startService(const std::string& id);

/** Stop a service.
 * @param[in] the service id as defined in its manifest
 * @return true on success or false when service wasn't running.
 */
bool stopService(const std::string& id);

/** Find a service manifest by its id.
 * @param[in] id the id as defined in the manifest
 * @return the matching manifest or nullptr when it wasn't found
 */
std::shared_ptr<const ServiceManifest> _Nullable findManifestId(const std::string& id);

/** Find a ServiceContext by its manifest id.
 * @param[in] id the id as defined in the manifest
 * @return the service context or nullptr when it wasn't found
 */
std::shared_ptr<ServiceContext> _Nullable findServiceContextById(const std::string& id);

/** Find a Service by its manifest id.
 * @param[in] id the id as defined in the manifest
 * @return the service context or nullptr when it wasn't found
 */
std::shared_ptr<Service> _Nullable findServiceById(const std::string& id);

/** Find a Service by its manifest id.
 * @param[in] id the id as defined in the manifest
 * @return the service context or nullptr when it wasn't found
 */
template <typename T>
std::shared_ptr<T> _Nullable findServiceById(const std::string& id) {
    return std::static_pointer_cast<T>(findServiceById(id));
}

} // namespace
