// SPDX-License-Identifier: Apache-2.0
#include <app/module.h>

#include <service/manager.h>

#include <tactility/error.h>
#include <tactility/module.h>

extern "C" {

extern ServiceManifest app_internal_loader_service_manifest;

static error_t start() {
    return service_manager_add(&app_internal_loader_service_manifest, /*auto_start=*/true);
}

static error_t stop() {
    return service_manager_remove(app_internal_loader_service_manifest.id);
}

Module app_module = {
    .name = "app",
    .start = start,
    .stop = stop,
    .drivers = nullptr,
    .symbols = nullptr,
    .internal = nullptr
};

}
