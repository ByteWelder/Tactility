#include <Tactility/Tactility.h>
#include <Tactility/TactilityConfig.h>

#include <Tactility/app/AppRegistration.h>
#include <Tactility/DispatcherThread.h>
#include <Tactility/hal/HalPrivate.h>
#include <Tactility/hal/sdcard/SdCardMounting.h>
#include <Tactility/lvgl/LvglPrivate.h>
#include <Tactility/network/NtpPrivate.h>
#include <Tactility/service/ServiceManifest.h>
#include <Tactility/service/ServiceRegistration.h>
#include <Tactility/service/loader/Loader.h>
#include <Tactility/settings/TimePrivate.h>

#ifdef ESP_PLATFORM
#include <Tactility/InitEsp.h>
#endif

namespace tt {

constexpr auto* TAG = "Tactility";

static const Configuration* config_instance = nullptr;
static Dispatcher mainDispatcher;

// region Default services
namespace service {
    // Primary
    namespace gps { extern const ServiceManifest manifest; }
    namespace wifi { extern const ServiceManifest manifest; }
    namespace sdcard { extern const ServiceManifest manifest; }
#ifdef ESP_PLATFORM
    namespace development { extern const ServiceManifest manifest; }
    namespace espnow { extern const ServiceManifest manifest; }
#endif
    // Secondary (UI)
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
    namespace addgps { extern const AppManifest manifest; }
    namespace alertdialog { extern const AppManifest manifest; }
    namespace applist { extern const AppManifest manifest; }
    namespace boot { extern const AppManifest manifest; }
    namespace calculator { extern const AppManifest manifest; }
    namespace chat { extern const AppManifest manifest; }
    namespace development { extern const AppManifest manifest; }
    namespace display { extern const AppManifest manifest; }
    namespace filebrowser { extern const AppManifest manifest; }
    namespace fileselection { extern const AppManifest manifest; }
    namespace gpio { extern const AppManifest manifest; }
    namespace gpssettings { extern const AppManifest manifest; }
    namespace i2cscanner { extern const AppManifest manifest; }
    namespace i2csettings { extern const AppManifest manifest; }
    namespace imageviewer { extern const AppManifest manifest; }
    namespace inputdialog { extern const AppManifest manifest; }
    namespace launcher { extern const AppManifest manifest; }
    namespace localesettings { extern const AppManifest manifest; }
    namespace log { extern const AppManifest manifest; }
    namespace notes { extern const AppManifest manifest; }
    namespace power { extern const AppManifest manifest; }
    namespace selectiondialog { extern const AppManifest manifest; }
    namespace serialconsole { extern const AppManifest manifest; }
    namespace settings { extern const AppManifest manifest; }
    namespace systeminfo { extern const AppManifest manifest; }
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

// List of all apps excluding Boot app (as Boot app calls this function indirectly)
static void registerSystemApps() {
    addApp(app::addgps::manifest);
    addApp(app::alertdialog::manifest);
    addApp(app::applist::manifest);
    addApp(app::calculator::manifest);
    addApp(app::display::manifest);
    addApp(app::filebrowser::manifest);
    addApp(app::fileselection::manifest);
    addApp(app::gpio::manifest);
    addApp(app::gpssettings::manifest);
    addApp(app::i2cscanner::manifest);
    addApp(app::i2csettings::manifest);
    addApp(app::imageviewer::manifest);
    addApp(app::inputdialog::manifest);
    addApp(app::launcher::manifest);
    addApp(app::localesettings::manifest);
    addApp(app::log::manifest);
    addApp(app::notes::manifest);
    addApp(app::serialconsole::manifest);
    addApp(app::settings::manifest);
    addApp(app::selectiondialog::manifest);
    addApp(app::systeminfo::manifest);
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
    addApp(app::development::manifest);
#endif

    if (hal::findDevices(hal::Device::Type::Power).size() > 0) {
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

static void registerAndStartSecondaryServices() {
    TT_LOG_I(TAG, "Registering and starting system services");
    addService(service::loader::manifest);
    addService(service::gui::manifest);
    addService(service::statusbar::manifest);
#if TT_FEATURE_SCREENSHOT_ENABLED
    addService(service::screenshot::manifest);
#endif
}

static void registerAndStartPrimaryServices() {
    TT_LOG_I(TAG, "Registering and starting system services");
    addService(service::gps::manifest);
    addService(service::sdcard::manifest);
    addService(service::wifi::manifest);
#ifdef ESP_PLATFORM
    addService(service::development::manifest);
    addService(service::espnow::manifest);
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
    TT_LOG_I(TAG, "Tactility v%s on %s (%s)", TT_VERSION, CONFIG_TT_BOARD_NAME, CONFIG_TT_BOARD_ID);

    assert(config.hardware);
    const hal::Configuration& hardware = *config.hardware;

    // Assign early so starting services can use it
    config_instance = &config;

#ifdef ESP_PLATFORM
    initEsp();
#endif
    settings::initTimeZone();
    hal::init(*config.hardware);
    network::ntp::init();

    registerAndStartPrimaryServices();
    lvgl::init(hardware);
    registerAndStartSecondaryServices();

    TT_LOG_I(TAG, "Core systems ready");

    TT_LOG_I(TAG, "Starting boot app");
    // The boot app takes care of registering system apps, user services and user apps
    addApp(app::boot::manifest);
    service::loader::startApp(app::boot::manifest.id);

    TT_LOG_I(TAG, "Main dispatcher ready");
    while (true) {
        mainDispatcher.consume();
    }
}

const Configuration* _Nullable getConfiguration() {
    return config_instance;
}

Dispatcher& getMainDispatcher() {
    return mainDispatcher;
}

} // namespace
