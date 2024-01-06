#include "tactility.h"

#include "app_manifest_registry.h"
#include "devices_i.h"
#include "furi.h"
#include "graphics_i.h"
#include "partitions.h"
#include "service_registry.h"

#define TAG "tactility"

// System services
extern const ServiceManifest gui_service;
extern const ServiceManifest loader_service;
extern const ServiceManifest desktop_service;

// System apps
extern const AppManifest system_info_app;

static void register_system_apps() {
    FURI_LOG_I(TAG, "Registering default apps");
    app_manifest_registry_add(&system_info_app);
}

static void register_user_apps(const Config* _Nonnull config) {
    FURI_LOG_I(TAG, "Registering user apps");
    for (size_t i = 0; i < CONFIG_APPS_LIMIT; i++) {
        const AppManifest* manifest = config->apps[i];
        if (manifest != NULL) {
            app_manifest_registry_add(manifest);
        } else {
            // reached end of list
            break;
        }
    }
}

static void register_system_services() {
    FURI_LOG_I(TAG, "Registering system services");
    service_registry_add(&gui_service);
    service_registry_add(&loader_service);
    service_registry_add(&desktop_service);
}

static void start_system_services() {
    FURI_LOG_I(TAG, "Starting system services");
    service_registry_start(gui_service.id);
    service_registry_start(loader_service.id);
    service_registry_start(desktop_service.id);
}

static void start_user_services(const Config* _Nonnull config) {
    FURI_LOG_I(TAG, "Starting user services");
    for (size_t i = 0; i < CONFIG_SERVICES_LIMIT; i++) {
        const ServiceManifest* manifest = config->services[i];
        if (manifest != NULL) {
            // TODO: keep track of running services
            manifest->on_start(NULL);
        } else {
            // reached end of list
            break;
        }
    }
}

__attribute__((unused)) extern void tactility_start(const Config* _Nonnull config) {
    furi_init();

    tt_partitions_init();

    Hardware hardware = tt_hardware_init(config->hardware);
    /*NbLvgl lvgl =*/tt_graphics_init(&hardware);

    // Register all apps
    register_system_services();
    register_system_apps();
    register_user_apps(config);

    // Start all services
    start_system_services();
    start_user_services(config);
}
