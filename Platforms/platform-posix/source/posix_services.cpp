// SPDX-License-Identifier: Apache-2.0

#include <tactility/posix_services.h>

#include <service/manager.h>

extern "C" {

extern const ServiceManifest posix_device_scanner_service_manifest;

error_t posix_services_start(void) {
    return service_manager_add(&posix_device_scanner_service_manifest, true);
}

error_t posix_services_stop(void) {
    const char* id = posix_device_scanner_service_manifest.id;
    error_t error = service_manager_stop(id);
    if (error != ERROR_NONE) return error;
    return service_manager_remove(id);
}

}
