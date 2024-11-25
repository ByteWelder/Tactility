#include "Tactility.h"

#include "AppManifestRegistry.h"
#include "LvglInit_i.h"
#include "service/ServiceRegistry.h"
#include "service/loader/Loader.h"
#include "TactilityHeadless.h"

namespace tt {

#define TAG "tactility"

static const Configuration* config_instance = NULL;

// region Default services

namespace service {
    namespace gui { extern const Manifest manifest; }
    namespace loader { extern const Manifest manifest; }
    namespace screenshot { extern const Manifest manifest; }
    namespace statusbar { extern const Manifest manifest; }
}

static const std::vector<const service::Manifest*> system_services = {
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
    namespace settings::i2c { extern const AppManifest manifest; }
    namespace settings::power { extern const AppManifest manifest; }
    namespace system_info { extern const AppManifest manifest; }
    namespace text_viewer { extern const AppManifest manifest; }
    namespace wifi_connect { extern const AppManifest manifest; }
    namespace wifi_manage { extern const AppManifest manifest; }
}

#ifndef ESP_PLATFORM
extern const AppManifest screenshot_app;
#endif

static const std::vector<const AppManifest*> system_apps = {
    &app::desktop::manifest,
    &app::files::manifest,
    &app::gpio::manifest,
    &app::image_viewer::manifest,
    &app::settings::manifest,
    &app::settings::display::manifest,
    &app::settings::i2c::manifest,
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
    for (const auto& app_manifest: system_apps) {
        app_manifest_registry_add(app_manifest);
    }

    if (getConfiguration()->hardware->power != nullptr) {
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
    for (const auto& service_manifest: system_services) {
        addService(service_manifest);
        tt_check(service::startService(service_manifest->id));
    }
}

static void register_and_start_user_services(const service::Manifest* const services[TT_CONFIG_SERVICES_LIMIT]) {
    TT_LOG_I(TAG, "Registering and starting user services");
    for (size_t i = 0; i < TT_CONFIG_SERVICES_LIMIT; i++) {
        const service::Manifest* manifest = services[i];
        if (manifest != nullptr) {
            addService(manifest);
            tt_check(service::startService(manifest->id));
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

    initHeadless(*config->hardware);

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

const Configuration* _Nullable getConfiguration() {
    return config_instance;
}

} // namespace
