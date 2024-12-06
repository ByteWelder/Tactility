#include <Dispatcher.h>
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
    namespace gui { extern const ServiceManifest manifest; }
    namespace loader { extern const ServiceManifest manifest; }
    namespace screenshot { extern const ServiceManifest manifest; }
    namespace statusbar { extern const ServiceManifest manifest; }
}

static const std::vector<const service::ServiceManifest*> system_services = {
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
    namespace boot { extern const AppManifest manifest; }
    namespace desktop { extern const AppManifest manifest; }
    namespace files { extern const AppManifest manifest; }
    namespace gpio { extern const AppManifest manifest; }
    namespace imageviewer { extern const AppManifest manifest; }
    namespace screenshot { extern const AppManifest manifest; }
    namespace settings { extern const AppManifest manifest; }
    namespace display { extern const AppManifest manifest; }
    namespace i2cscanner { extern const AppManifest manifest; }
    namespace i2csettings { extern const AppManifest manifest; }
    namespace power { extern const AppManifest manifest; }
    namespace selectiondialog { extern const AppManifest manifest; }
    namespace systeminfo { extern const AppManifest manifest; }
    namespace textviewer { extern const AppManifest manifest; }
    namespace wifiapsettings { extern const AppManifest manifest; }
    namespace wificonnect { extern const AppManifest manifest; }
    namespace wifimanage { extern const AppManifest manifest; }
}

#ifndef ESP_PLATFORM
extern const app::AppManifest screenshot_app;
#endif

static const std::vector<const app::AppManifest*> system_apps = {
    &app::boot::manifest,
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
    &app::wifiapsettings::manifest,
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

static void register_user_apps(const app::AppManifest* const apps[TT_CONFIG_APPS_LIMIT]) {
    TT_LOG_I(TAG, "Registering user apps");
    for (size_t i = 0; i < TT_CONFIG_APPS_LIMIT; i++) {
        const app::AppManifest* manifest = apps[i];
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

static void register_and_start_user_services(const service::ServiceManifest* const services[TT_CONFIG_SERVICES_LIMIT]) {
    TT_LOG_I(TAG, "Registering and starting user services");
    for (size_t i = 0; i < TT_CONFIG_SERVICES_LIMIT; i++) {
        const service::ServiceManifest* manifest = services[i];
        if (manifest != nullptr) {
            addService(manifest);
            tt_check(service::startService(manifest->id));
        } else {
            // reached end of list
            break;
        }
    }
}

void run(const Configuration& config) {
    TT_LOG_I(TAG, "init started");

    tt_assert(config.hardware);
    const hal::Configuration& hardware = *config.hardware;

    // Assign early so starting services can use it
    config_instance = &config;

    initHeadless(hardware);

    lvgl::init(hardware);

    // Note: the order of starting apps and services is critical!
    // System services are registered first so the apps below can find them if needed
    register_and_start_system_services();
    // Then we register system apps. They are not used/started yet.
    register_system_apps();
    // Then we register and start user services. They are started after system app
    // registration just in case they want to figure out which system apps are installed.
    register_and_start_user_services(config.services);
    // Now we register the user apps, as they might rely on the user services.
    register_user_apps(config.apps);

    TT_LOG_I(TAG, "init starting desktop app");
    service::loader::startApp(app::boot::manifest.id, true);

    if (config.auto_start_app_id) {
        TT_LOG_I(TAG, "init auto-starting %s", config.auto_start_app_id);
        service::loader::startApp(config.auto_start_app_id, true);
    }

    TT_LOG_I(TAG, "init complete");

    TT_LOG_I(TAG, "Processing main dispatcher");
    while (true) {
        getMainDispatcher().consume(TtWaitForever);
    }
}

const Configuration* _Nullable getConfiguration() {
    return config_instance;
}

} // namespace
