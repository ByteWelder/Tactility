#include "Tactility.h"

#include "app/ManifestRegistry.h"
#include "service/ServiceRegistry.h"
#include "service/loader/Loader.h"
#include "TactilityHeadless.h"
#include "lvgl/Init_i.h"

namespace tt {

#define TAG "tactility"

static const Configuration* config_instance = nullptr;

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
    namespace desktop { extern const Manifest manifest; }
    namespace files { extern const Manifest manifest; }
    namespace gpio { extern const Manifest manifest; }
    namespace imageviewer { extern const Manifest manifest; }
    namespace screenshot { extern const Manifest manifest; }
    namespace settings { extern const Manifest manifest; }
    namespace display { extern const Manifest manifest; }
    namespace i2cscanner { extern const Manifest manifest; }
    namespace i2csettings { extern const Manifest manifest; }
    namespace power { extern const Manifest manifest; }
    namespace selectiondialog { extern const Manifest manifest; }
    namespace systeminfo { extern const Manifest manifest; }
    namespace textviewer { extern const Manifest manifest; }
    namespace wificonnect { extern const Manifest manifest; }
    namespace wifimanage { extern const Manifest manifest; }
}

#ifndef ESP_PLATFORM
extern const app::Manifest screenshot_app;
#endif

static const std::vector<const app::Manifest*> system_apps = {
    &app::desktop::manifest,
    &app::display::manifest,
    &app::files::manifest,
    &app::gpio::manifest,
    &app::i2cscanner::manifest,
    &app::i2csettings::manifest,
    &app::imageviewer::manifest,
    &app::settings::manifest,
    &app::selectiondialog::manifest,
    &app::systeminfo::manifest,
    &app::textviewer::manifest,
    &app::wificonnect::manifest,
    &app::wifimanage::manifest,
#ifndef ESP_PLATFORM
    &app::screenshot::manifest, // Screenshots don't work yet on ESP32
#endif
};

// endregion

static void register_system_apps() {
    TT_LOG_I(TAG, "Registering default apps");
    for (const auto& app_manifest: system_apps) {
        addApp(app_manifest);
    }

    if (getConfiguration()->hardware->power != nullptr) {
        addApp(&app::power::manifest);
    }
}

static void register_user_apps(const app::Manifest* const apps[TT_CONFIG_APPS_LIMIT]) {
    TT_LOG_I(TAG, "Registering user apps");
    for (size_t i = 0; i < TT_CONFIG_APPS_LIMIT; i++) {
        const app::Manifest* manifest = apps[i];
        if (manifest != nullptr) {
            addApp(manifest);
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

    lvgl::init(config->hardware);

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
