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
    namespace statusbar { extern const ServiceManifest manifest; }
#if TT_FEATURE_SCREENSHOT_ENABLED
    namespace screenshot { extern const ServiceManifest manifest; }
#endif
}

// endregion

// region Default apps

namespace app {
    namespace alertdialog { extern const AppManifest manifest; }
    namespace applist { extern const AppManifest manifest; }
    namespace boot { extern const AppManifest manifest; }
    namespace files { extern const AppManifest manifest; }
    namespace gpio { extern const AppManifest manifest; }
    namespace display { extern const AppManifest manifest; }
    namespace i2cscanner { extern const AppManifest manifest; }
    namespace i2csettings { extern const AppManifest manifest; }
    namespace imageviewer { extern const AppManifest manifest; }
    namespace inputdialog { extern const AppManifest manifest; }
    namespace launcher { extern const AppManifest manifest; }
    namespace log { extern const AppManifest manifest; }
    namespace power { extern const AppManifest manifest; }
    namespace selectiondialog { extern const AppManifest manifest; }
    namespace settings { extern const AppManifest manifest; }
    namespace systeminfo { extern const AppManifest manifest; }
    namespace textviewer { extern const AppManifest manifest; }
    namespace timedatesettings { extern const AppManifest manifest; }
    namespace timezone { extern const AppManifest manifest; }
    namespace usbsettings { extern const AppManifest manifest; }
    namespace wifiapsettings { extern const AppManifest manifest; }
    namespace wificonnect { extern const AppManifest manifest; }
    namespace wifimanage { extern const AppManifest manifest; }
#if TT_FEATURE_SCREENSHOT_ENABLED
        namespace screenshot { extern const AppManifest manifest; }
#endif
#ifdef ESP_PLATFORM
    namespace crashdiagnostics { extern const AppManifest manifest; }
#endif
}

#ifndef ESP_PLATFORM
#endif

static const std::vector<const app::AppManifest*> system_apps = {
    &app::alertdialog::manifest,
    &app::applist::manifest,
    &app::boot::manifest,
    &app::display::manifest,
    &app::files::manifest,
    &app::gpio::manifest,
    &app::i2cscanner::manifest,
    &app::i2csettings::manifest,
    &app::imageviewer::manifest,
    &app::inputdialog::manifest,
    &app::launcher::manifest,
    &app::log::manifest,
    &app::settings::manifest,
    &app::selectiondialog::manifest,
    &app::systeminfo::manifest,
    &app::textviewer::manifest,
    &app::timedatesettings::manifest,
    &app::timezone::manifest,
    &app::usbsettings::manifest,
    &app::wifiapsettings::manifest,
    &app::wificonnect::manifest,
    &app::wifimanage::manifest,
#if TT_FEATURE_SCREENSHOT_ENABLED
    &app::screenshot::manifest,
#endif
#ifdef ESP_PLATFORM
    &app::crashdiagnostics::manifest
#endif
};

// endregion

static void register_system_apps() {
    TT_LOG_I(TAG, "Registering default apps");
    for (const auto* app_manifest: system_apps) {
        addApp(*app_manifest);
    }

    if (getConfiguration()->hardware->power != nullptr) {
        addApp(app::power::manifest);
    }
}

static void register_user_apps(const std::vector<const app::AppManifest*>& apps) {
    TT_LOG_I(TAG, "Registering user apps");
    for (auto* manifest : apps) {
        assert(manifest != nullptr);
        addApp(*manifest);
    }
}

static void register_and_start_system_services() {
    TT_LOG_I(TAG, "Registering and starting system services");
    addService(service::loader::manifest);
    addService(service::gui::manifest);
    addService(service::statusbar::manifest);
#if TT_FEATURE_SCREENSHOT_ENABLED
    addService(service::screenshot::manifest);
#endif
}

static void register_and_start_user_services(const std::vector<const service::ServiceManifest*>& manifests) {
    TT_LOG_I(TAG, "Registering and starting user services");
    for (auto* manifest : manifests) {
        assert(manifest != nullptr);
        addService(*manifest);
    }
}

void run(const Configuration& config) {
    TT_LOG_D(TAG, "run");

    assert(config.hardware);
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
    service::loader::startApp(app::boot::manifest.id);

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
