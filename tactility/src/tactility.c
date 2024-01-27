#include "tactility.h"

#include "app_manifest_registry.h"
#include "hardware_i.h"
#include "service_registry.h"
#include "services/loader/loader.h"

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

static void register_user_apps(const AppManifest* const apps[TT_CONFIG_APPS_LIMIT]) {
    TT_LOG_I(TAG, "Registering user apps");
    for (size_t i = 0; i < TT_CONFIG_APPS_LIMIT; i++) {
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

static void register_and_start_user_services(const ServiceManifest* const services[TT_CONFIG_SERVICES_LIMIT]) {
    TT_LOG_I(TAG, "Registering and starting user services");
    for (size_t i = 0; i < TT_CONFIG_SERVICES_LIMIT; i++) {
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

TT_UNUSED void tt_init(const Config* config) {
    TT_LOG_I(TAG, "tt_init started");

    tt_service_registry_init();
    tt_app_manifest_registry_init();

    tt_hardware_init(config->hardware);

    // Register all apps
    register_system_services();
    register_system_apps();
    // TODO: move this after start_system_services, but desktop must subscribe to app registry events first.
    register_user_apps(config->apps);

    // Start all services
    start_system_services();
    register_and_start_user_services(config->services);

    TT_LOG_I(TAG, "tt_init starting desktop app");
    loader_start_app(desktop_app.id, true, NULL);

    if (config->auto_start_app_id) {
        TT_LOG_I(TAG, "tt_init aut-starting %s", config->auto_start_app_id);
        loader_start_app(config->auto_start_app_id, true, NULL);
    }

    TT_LOG_I(TAG, "tt_init complete");
}
