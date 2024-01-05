#include "nanobake.h"
#include "app_manifest_registry.h"
#include "devices_i.h"
#include "furi.h"
#include "graphics_i.h"
#include "partitions.h"
#include "apps/services/gui/gui.h"
#define TAG "nanobake"

Gui* gui_alloc();

// System services
extern const AppManifest gui_app;
extern const AppManifest loader_app;

// Desktop
extern const AppManifest desktop_app;

// System apps
extern const AppManifest system_info_app;

void start_service(const AppManifest* _Nonnull manifest, void* _Nullable context) {
    UNUSED(context);
    FURI_LOG_I(TAG, "Starting service %s", manifest->name);
    furi_check(manifest->on_start, "service must define on_start");
    manifest->on_start(NULL);
    // TODO: keep track of running services
}

static void register_apps(Config* _Nonnull config) {
    FURI_LOG_I(TAG, "Registering core apps");
    app_manifest_registry_add(&gui_app);
    app_manifest_registry_add(&desktop_app);
    app_manifest_registry_add(&loader_app);
    app_manifest_registry_add(&system_info_app);

    FURI_LOG_I(TAG, "Registering user apps");
    for (size_t i = 0; i < config->apps_count; i++) {
        app_manifest_registry_add(config->apps[i]);
    }
}

static void start_services() {
    FURI_LOG_I(TAG, "Starting services");
    app_manifest_registry_for_each_of_type(AppTypeService, NULL, start_service);
    FURI_LOG_I(TAG, "Startup complete");
}

static void start_desktop() {
    FURI_LOG_I(TAG, "Starting desktop");
    desktop_app.on_start(NULL);
    FURI_LOG_I(TAG, "Startup complete");
}

__attribute__((unused)) extern void nanobake_start(const Config* _Nonnull config) {
    furi_init();

    nb_partitions_init();

    Hardware hardware = nb_hardware_init(config->hardware);
    /*NbLvgl lvgl =*/nb_graphics_init(&hardware);

    register_apps(config);

    start_services();
    start_desktop();
}
