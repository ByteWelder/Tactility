#include "nanobake.h"
#include "app_manifest_registry.h"
#include "devices_i.h"
#include "furi.h"
#include "graphics_i.h"
#include "partitions.h"
#include "services/gui/gui.h"

#define TAG "nanobake"

Gui* gui_alloc();

// System services
extern const ServiceManifest gui_service;
extern const ServiceManifest loader_service;
extern const ServiceManifest desktop_service;
extern const ServiceManifest wifi_service;

// System apps
extern const AppManifest system_info_app;

void start_service(const ServiceManifest* _Nonnull manifest) {
    FURI_LOG_I(TAG, "Starting service %s", manifest->id);
    furi_check(manifest->on_start, "service must define on_start");
    manifest->on_start(NULL);
    // TODO: keep track of running services
}

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

static void start_system_services() {
    FURI_LOG_I(TAG, "Starting system services");
    start_service(&gui_service);
    start_service(&loader_service);
    start_service(&desktop_service);
    start_service(&wifi_service);
    FURI_LOG_I(TAG, "System services started");
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
    FURI_LOG_I(TAG, "User services started");
}

__attribute__((unused)) extern void nanobake_start(const Config* _Nonnull config) {
    furi_init();

    nb_partitions_init();

    Hardware hardware = nb_hardware_init(config->hardware);
    /*NbLvgl lvgl =*/nb_graphics_init(&hardware);

    // Register all apps
    register_system_apps();
    register_user_apps(config);

    // Start all services
    start_system_services();
    start_user_services(config);
}