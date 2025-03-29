#include "Tactility/Tactility.h"

#include "Tactility/app/ManifestRegistry.h"
#include "Tactility/lvgl/Init_i.h"
#include "Tactility/service/ServiceManifest.h"

#include <Tactility/TactilityHeadless.h>
#include <Tactility/service/ServiceRegistry.h>
#include <Tactility/service/loader/Loader.h>

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
    namespace chat { extern const AppManifest manifest; }
    namespace boot { extern const AppManifest manifest; }
    namespace display { extern const AppManifest manifest; }
    namespace files { extern const AppManifest manifest; }
    namespace gpio { extern const AppManifest manifest; }
    namespace gpssettings { extern const AppManifest manifest; }
    namespace i2cscanner { extern const AppManifest manifest; }
    namespace i2csettings { extern const AppManifest manifest; }
    namespace imageviewer { extern const AppManifest manifest; }
    namespace inputdialog { extern const AppManifest manifest; }
    namespace launcher { extern const AppManifest manifest; }
    namespace log { extern const AppManifest manifest; }
    namespace power { extern const AppManifest manifest; }
    namespace selectiondialog { extern const AppManifest manifest; }
    namespace serialconsole { extern const AppManifest manifest; }
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

// endregion

static void registerSystemApps() {
    addApp(app::alertdialog::manifest);
    addApp(app::applist::manifest);
    addApp(app::display::manifest);
    addApp(app::files::manifest);
    addApp(app::gpio::manifest);
    addApp(app::gpssettings::manifest);
    addApp(app::i2cscanner::manifest);
    addApp(app::i2csettings::manifest);
    addApp(app::imageviewer::manifest);
    addApp(app::inputdialog::manifest);
    addApp(app::launcher::manifest);
    addApp(app::log::manifest);
    addApp(app::serialconsole::manifest);
    addApp(app::settings::manifest);
    addApp(app::selectiondialog::manifest);
    addApp(app::systeminfo::manifest);
    addApp(app::textviewer::manifest);
    addApp(app::timedatesettings::manifest);
    addApp(app::timezone::manifest);
    addApp(app::usbsettings::manifest);
    addApp(app::wifiapsettings::manifest);
    addApp(app::wificonnect::manifest);
    addApp(app::wifimanage::manifest);

#if TT_FEATURE_SCREENSHOT_ENABLED
    addApp(app::screenshot::manifest);
#endif

#ifdef ESP_PLATFORM
    addApp(app::chat::manifest);
    addApp(app::crashdiagnostics::manifest);
#endif

    if (getConfiguration()->hardware->power != nullptr) {
        addApp(app::power::manifest);
    }
}

static void registerUserApps(const std::vector<const app::AppManifest*>& apps) {
    TT_LOG_I(TAG, "Registering user apps");
    for (auto* manifest : apps) {
        assert(manifest != nullptr);
        addApp(*manifest);
    }
}

static void registerAndStartSystemServices() {
    TT_LOG_I(TAG, "Registering and starting system services");
    addService(service::loader::manifest);
    addService(service::gui::manifest);
    addService(service::statusbar::manifest);
#if TT_FEATURE_SCREENSHOT_ENABLED
    addService(service::screenshot::manifest);
#endif
}

static void registerAndStartUserServices(const std::vector<const service::ServiceManifest*>& manifests) {
    TT_LOG_I(TAG, "Registering and starting user services");
    for (auto* manifest : manifests) {
        assert(manifest != nullptr);
        addService(*manifest);
    }
}

void initFromBootApp() {
    auto configuration = getConfiguration();
    // Then we register system apps. They are not used/started yet.
    registerSystemApps();
    // Then we register and start user services. They are started after system app
    // registration just in case they want to figure out which system apps are installed.
    registerAndStartUserServices(configuration->services);
    // Now we register the user apps, as they might rely on the user services.
    registerUserApps(configuration->apps);
}

void run(const Configuration& config) {
    TT_LOG_D(TAG, "run");

    assert(config.hardware);
    const hal::Configuration& hardware = *config.hardware;

    // Assign early so starting services can use it
    config_instance = &config;

    initHeadless(hardware);

    lvgl::init(hardware);

    registerAndStartSystemServices();

    TT_LOG_I(TAG, "starting boot app");
    // The boot app takes care of registering system apps, user services and user apps
    addApp(app::boot::manifest);
    service::loader::startApp(app::boot::manifest.id);

    TT_LOG_I(TAG, "init complete");

    TT_LOG_I(TAG, "Processing main dispatcher");
    while (true) {
        getMainDispatcher().consume();
    }
}

const Configuration* _Nullable getConfiguration() {
    return config_instance;
}

} // namespace
