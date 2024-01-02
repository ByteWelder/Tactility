#include "nanobake.h"
#include "app_i.h"
#include "app_manifest_registry.h"
#include "devices_i.h"
#include "furi.h"
#include "graphics_i.h"
#include "partitions.h"

#define TAG "nanobake"

// System services
extern const AppManifest desktop_app;
extern const AppManifest gui_app;
extern const AppManifest loader_app;

// System apps
extern const AppManifest system_info_app;

void start_service(const AppManifest* _Nonnull manifest) {
    // TODO: keep track of running services
    FURI_LOG_I(TAG, "Starting service %s", manifest->name);
    manifest->on_start(NULL);
}

static void register_apps(Config* _Nonnull config) {
    FURI_LOG_I(TAG, "Registering core apps");
    app_manifest_registry_add(&desktop_app);
    app_manifest_registry_add(&gui_app);
    app_manifest_registry_add(&loader_app);
    app_manifest_registry_add(&system_info_app);

    FURI_LOG_I(TAG, "Registering user apps");
    for (size_t i = 0; i < config->apps_count; i++) {
        app_manifest_registry_add(config->apps[i]);
    }
}

static void start_services() {
    FURI_LOG_I(TAG, "Starting services");
    app_manifest_registry_for_each_of_type(AppTypeService, start_service);
    FURI_LOG_I(TAG, "Startup complete");
}

__attribute__((unused)) extern void nanobake_start(Config* _Nonnull config) {
    furi_init();

    nb_partitions_init();

    Devices hardware = nb_devices_create(config);
    /*NbLvgl lvgl =*/nb_graphics_init(&hardware);

    register_apps(config);

    start_services();
    // TODO: option to await starting services?
}
