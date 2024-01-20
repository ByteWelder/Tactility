#include "tactility.h"

#include "app_manifest_registry.h"
#include "core.h"
#include "service_registry.h"

#define TAG "tactility"

// System services
extern const ServiceManifest gui_service;
extern const ServiceManifest loader_service;

// System apps
extern const AppManifest desktop_app;
extern const AppManifest system_info_app;

static void register_system_apps() {
    TT_LOG_I(TAG, "Registering default apps");
    tt_app_manifest_registry_add(&desktop_app);
    tt_app_manifest_registry_add(&system_info_app);
}

static void register_user_apps(
    const AppManifest* const* _Nonnull apps,
    size_t apps_count
) {
    TT_LOG_I(TAG, "Registering user apps");
    for (size_t i = 0; i < apps_count; i++) {
        const AppManifest* manifest = apps[i];
        if (manifest != NULL) {
            tt_app_manifest_registry_add(manifest);
        } else {
            // reached end of list
            break;
        }
    }
}

static void register_system_services() {
    TT_LOG_I(TAG, "Registering system services");
    tt_service_registry_add(&gui_service);
    tt_service_registry_add(&loader_service);
}

static void start_system_services() {
    TT_LOG_I(TAG, "Starting system services");
    tt_service_registry_start(gui_service.id);
    tt_service_registry_start(loader_service.id);
}

static void register_and_start_user_services(
    const ServiceManifest* const* services,
    size_t servics_count
) {
    TT_LOG_I(TAG, "Registering and starting user services");
    for (size_t i = 0; i < servics_count; i++) {
        const ServiceManifest* manifest = services[i];
        if (manifest != NULL) {
            tt_service_registry_add(manifest);
            tt_service_registry_start(manifest->id);
        } else {
            // reached end of list
            break;
        }
    }
}

__attribute__((unused)) void tt_init(
    const AppManifest* const* _Nonnull apps,
    size_t apps_count,
    const ServiceManifest* const* services,
    size_t services_count
) {
    TT_LOG_I(TAG, "tt_init started");
    // Register all apps
    register_system_services();
    register_system_apps();
    // TODO: move this after start_system_services, but desktop must subscribe to app registry events first.
    register_user_apps(apps, apps_count);

    // Start all services
    start_system_services();
    register_and_start_user_services(services, services_count);
    TT_LOG_I(TAG, "tt_init complete");
}

