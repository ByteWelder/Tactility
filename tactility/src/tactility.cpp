#include "tactility.h"

#include "app_manifest_registry.h"
#include "lvgl_init_i.h"
#include "service_registry.h"
#include "services/loader/loader.h"
#include "tactility_headless.h"

namespace tt {

#define TAG "tactility"

static const Configuration* config_instance = NULL;

// region Default services

namespace service {
    namespace gui { extern const ServiceManifest manifest; }
    namespace loader { extern const ServiceManifest manifest; }
    namespace screenshot { extern const ServiceManifest manifest; }
    namespace statusbar { extern const ServiceManifest manifest; }
}

static const ServiceManifest* const system_services[] = {
    &service::loader::manifest,
    &service::gui::manifest, // depends on loader service
#ifndef ESP_PLATFORM // Screenshots don't work yet on ESP32
    &service::screenshot::manifest,
#endif
    &service::statusbar::manifest
};

// endregion

// region Default apps

namespace app {
    namespace desktop { extern const AppManifest manifest; }
    namespace files { extern const AppManifest manifest; }
    namespace gpio { extern const AppManifest manifest; }
    namespace image_viewer { extern const AppManifest manifest; }
    namespace screenshot { extern const AppManifest manifest; }
    namespace settings { extern const AppManifest manifest; }
    namespace settings::display { extern const AppManifest manifest; }
    namespace settings::power { extern const AppManifest manifest; }
    namespace system_info { extern const AppManifest manifest; }
    namespace text_viewer { extern const AppManifest manifest; }
    namespace wifi_connect { extern const AppManifest manifest; }
    namespace wifi_manage { extern const AppManifest manifest; }
}

#ifndef ESP_PLATFORM
extern const AppManifest screenshot_app;
#endif

static const AppManifest* const system_apps[] = {
    &app::desktop::manifest,
    &app::files::manifest,
    &app::gpio::manifest,
    &app::image_viewer::manifest,
    &app::settings::manifest,
    &app::settings::display::manifest,
    &app::system_info::manifest,
    &app::text_viewer::manifest,
    &app::wifi_connect::manifest,
    &app::wifi_manage::manifest,
#ifndef ESP_PLATFORM
    &app::screenshot::manifest, // Screenshots don't work yet on ESP32
#endif
};

// endregion

static void register_system_apps() {
    TT_LOG_I(TAG, "Registering default apps");

    int app_count = sizeof(system_apps) / sizeof(AppManifest*);
    for (int i = 0; i < app_count; ++i) {
        app_manifest_registry_add(system_apps[i]);
    }

    if (get_config()->hardware->power != nullptr) {
        app_manifest_registry_add(&app::settings::power::manifest);
    }
}

static void register_user_apps(const AppManifest* const apps[TT_CONFIG_APPS_LIMIT]) {
    TT_LOG_I(TAG, "Registering user apps");
    for (size_t i = 0; i < TT_CONFIG_APPS_LIMIT; i++) {
        const AppManifest* manifest = apps[i];
        if (manifest != nullptr) {
            app_manifest_registry_add(manifest);
        } else {
            // reached end of list
            break;
        }
    }
}

static void register_and_start_system_services() {
    TT_LOG_I(TAG, "Registering and starting system services");
    int app_count = sizeof(system_services) / sizeof(ServiceManifest*);
    for (int i = 0; i < app_count; ++i) {
        service_registry_add(system_services[i]);
        tt_check(service_registry_start(system_services[i]->id));
    }
}

static void register_and_start_user_services(const ServiceManifest* const services[TT_CONFIG_SERVICES_LIMIT]) {
    TT_LOG_I(TAG, "Registering and starting user services");
    for (size_t i = 0; i < TT_CONFIG_SERVICES_LIMIT; i++) {
        const ServiceManifest* manifest = services[i];
        if (manifest != nullptr) {
            service_registry_add(manifest);
            tt_check(service_registry_start(manifest->id));
        } else {
            // reached end of list
            break;
        }
    }
}

void init(const Configuration* config) {
    TT_LOG_I(TAG, "init started");


    // Assign early so starting services can use it
    config_instance = config;

    headless_init(config->hardware);

    lvgl_init(config->hardware);

    app_manifest_registry_init();

    // Note: the order of starting apps and services is critical!
    // System services are registered first so the apps below can find them if needed
    register_and_start_system_services();
    // Then we register system apps. They are not used/started yet.
    register_system_apps();
    // Then we register and start user services. They are started after system app
    // registration just in case they want to figure out which system apps are installed.
    register_and_start_user_services(config->services);
    // Now we register the user apps, as they might rely on the user services.
    register_user_apps(config->apps);

    TT_LOG_I(TAG, "init starting desktop app");
    service::loader::start_app(app::desktop::manifest.id, true, Bundle());

    if (config->auto_start_app_id) {
        TT_LOG_I(TAG, "init auto-starting %s", config->auto_start_app_id);
        service::loader::start_app(config->auto_start_app_id, true, Bundle());
    }

    TT_LOG_I(TAG, "init complete");
}

const Configuration* _Nullable get_config() {
    return config_instance;
}

} // namespace
