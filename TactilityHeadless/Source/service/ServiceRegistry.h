#pragma once

#include "service/ServiceManifest.h"

namespace tt::service {

void initRegistry();

/** Register a service.
 * @param[in] the service manifest
 */
void addService(const ServiceManifest* manifest);

/** Unregister a service.
 * @param[in] the service manifest
 */
void removeService(const ServiceManifest* manifest);

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
const ServiceManifest* _Nullable findManifestId(const std::string& id);

/** Find a service by its manifest id.
 * @param[in] id the id as defined in the manifest
 * @return the service context or nullptr when it wasn't found
 */
ServiceContext* _Nullable findServiceById(const std::string& id);

} // namespace
