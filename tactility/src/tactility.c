#include "tactility.h"

#include "app_manifest_registry.h"
#include "hardware_i.h"
#include "preferences.h"
#include "service_registry.h"
#include "services/loader/loader.h"

#define TAG "tactility"

static const Config* config_instance = NULL;

// region Default services

extern const ServiceManifest gui_service;
extern const ServiceManifest loader_service;

static const ServiceManifest* const system_services[] = {
    &gui_service,
    &loader_service // depends on gui service
};

// endregion

// region Default apps

extern const AppManifest desktop_app;
extern const AppManifest display_app;
extern const AppManifest files_app;
extern const AppManifest settings_app;
extern const AppManifest system_info_app;

static const AppManifest* const system_apps[] = {
    &desktop_app,
    &display_app,
    &files_app,
    &settings_app,
    &system_info_app
};

// endregion

static void register_system_apps() {
    TT_LOG_I(TAG, "Registering default apps");
    int app_count = sizeof(system_apps) / sizeof(AppManifest*);
    for (int i = 0; i < app_count; ++i) {
        tt_app_manifest_registry_add(system_apps[i]);
    }
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

static void register_and_start_system_services() {
    TT_LOG_I(TAG, "Registering and starting system services");
    int app_count = sizeof(system_services) / sizeof(ServiceManifest *);
    for (int i = 0; i < app_count; ++i) {
        tt_service_registry_add(system_services[i]);
        tt_check(tt_service_registry_start(system_services[i]->id));
    }
}

static void register_and_start_user_services(const ServiceManifest* const services[TT_CONFIG_SERVICES_LIMIT]) {
    TT_LOG_I(TAG, "Registering and starting user services");
    for (size_t i = 0; i < TT_CONFIG_SERVICES_LIMIT; i++) {
        const ServiceManifest* manifest = services[i];
        if (manifest != NULL) {
            tt_service_registry_add(manifest);
            tt_check(tt_service_registry_start(manifest->id));
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

    SetBacklightDuty set_backlight_duty = config->hardware->display.set_backlight_duty;
    if (set_backlight_duty != NULL) {
        int32_t backlight_duty = 200;
        if (!tt_preferences()->opt_int32("display", "backlight_duty", &backlight_duty)) {
            tt_preferences()->put_int32("display", "backlight_duty", backlight_duty);
        }
        int32_t safe_backlight_duty = TT_MIN(backlight_duty, 255);
        set_backlight_duty((uint8_t)safe_backlight_duty);
    }

    // Note: the order of starting apps and services is critical!
    // System services are registered first so the apps below can use them
    register_and_start_system_services();
    // Then we register system apps. They are not used/started yet.
    register_system_apps();
    // Then we register and start user services. They are started after system app
    // registration just in case they want to figure out which system apps are installed.
    register_and_start_user_services(config->services);
    // Now we register the user apps, as they might rely on the user services.
    register_user_apps(config->apps);

    TT_LOG_I(TAG, "tt_init starting desktop app");
    loader_start_app(desktop_app.id, true, NULL);

    if (config->auto_start_app_id) {
        TT_LOG_I(TAG, "tt_init auto-starting %s", config->auto_start_app_id);
        loader_start_app(config->auto_start_app_id, true, NULL);
    }

    TT_LOG_I(TAG, "tt_init complete");

    config_instance = config;
}

const Config* _Nullable tt_get_config() {
    return config_instance;
}
