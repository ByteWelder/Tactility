#include "Tactility/app/AppManifestParsing.h"

#include <Tactility/Tactility.h>
#include <Tactility/TactilityConfig.h>
#include <Tactility/app/AppRegistration.h>
#include <Tactility/DispatcherThread.h>
#include <Tactility/MountPoints.h>
#include <Tactility/file/File.h>
#include <Tactility/file/PropertiesFile.h>
#include <Tactility/hal/HalPrivate.h>
#include <Tactility/lvgl/LvglPrivate.h>
#include <Tactility/network/NtpPrivate.h>
#include <Tactility/service/ServiceManifest.h>
#include <Tactility/service/ServiceRegistration.h>
#include <Tactility/service/loader/Loader.h>
#include <Tactility/settings/TimePrivate.h>

#include <map>
#include <format>

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
    namespace files { extern const AppManifest manifest; }
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
    addApp(app::alertdialog::manifest);
    addApp(app::applist::manifest);
    addApp(app::calculator::manifest);
    addApp(app::display::manifest);
    addApp(app::files::manifest);
    addApp(app::fileselection::manifest);
    addApp(app::gpio::manifest);
    addApp(app::imageviewer::manifest);
    addApp(app::inputdialog::manifest);
    addApp(app::launcher::manifest);
    addApp(app::localesettings::manifest);
    addApp(app::log::manifest);
    addApp(app::notes::manifest);
    addApp(app::settings::manifest);
    addApp(app::selectiondialog::manifest);
    addApp(app::systeminfo::manifest);
    addApp(app::timedatesettings::manifest);
    addApp(app::timezone::manifest);
    addApp(app::wifiapsettings::manifest);
    addApp(app::wificonnect::manifest);
    addApp(app::wifimanage::manifest);

#if defined(CONFIG_TINYUSB_MSC_ENABLED) && CONFIG_TINYUSB_MSC_ENABLED
    addApp(app::usbsettings::manifest);
#endif

#if TT_FEATURE_SCREENSHOT_ENABLED
    addApp(app::screenshot::manifest);
#endif

#ifdef ESP_PLATFORM
    addApp(app::chat::manifest);
    addApp(app::crashdiagnostics::manifest);
    addApp(app::development::manifest);
#endif

    if (!hal::getConfiguration()->i2c.empty()) {
        addApp(app::i2cscanner::manifest);
        addApp(app::i2csettings::manifest);
    }

    if (!hal::getConfiguration()->uart.empty()) {
        addApp(app::addgps::manifest);
        addApp(app::gpssettings::manifest);
        addApp(app::serialconsole::manifest);
    }

    if (hal::hasDevice(hal::Device::Type::Power)) {
        addApp(app::power::manifest);
    }
}

static void registerInstalledApp(std::string path) {
    TT_LOG_I(TAG, "Registering app at %s", path.c_str());
    std::string manifest_path = path + "/manifest.properties";
    if (!file::isFile(manifest_path)) {
        TT_LOG_E(TAG, "Manifest not found at %s", manifest_path.c_str());
        return;
    }

    std::map<std::string, std::string> properties;
    if (!file::loadPropertiesFile(manifest_path, properties)) {
        TT_LOG_E(TAG, "Failed to load manifest at %s", manifest_path.c_str());
        return;
    }

    app::AppManifest manifest;
    if (!app::parseManifest(properties, manifest)) {
        TT_LOG_E(TAG, "Failed to parse manifest at %s", manifest_path.c_str());
        return;
    }

    manifest.appCategory = app::Category::User;
    manifest.appLocation = app::Location::external(path);

    app::addApp(manifest);
}

static void registerInstalledApps(const std::string& path) {
    file::listDirectory(path, [&path](const auto& entry) {
        auto absolute_path = std::format("{}/{}", path, entry.d_name);
        if (file::isDirectory(absolute_path)) {
            registerInstalledApp(absolute_path);
        }
    });
}

static void registerInstalledAppsFromSdCard(const std::shared_ptr<hal::sdcard::SdCardDevice>& sdcard) {
    auto sdcard_root_path = sdcard->getMountPath();
    auto app_path = std::format("{}/app", sdcard_root_path);
    sdcard->getLock()->lock();
    if (file::isDirectory(app_path)) {
        registerInstalledApps(app_path);
    }
    sdcard->getLock()->unlock();
}

static void registerInstalledAppsFromSdCards() {
    auto sdcard_devices = hal::findDevices<hal::sdcard::SdCardDevice>(hal::Device::Type::SdCard);
    for (const auto& sdcard : sdcard_devices) {
        if (sdcard->isMounted()) {
            registerInstalledAppsFromSdCard(sdcard);
        }
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
    // Then we register apps. They are not used/started yet.
    registerSystemApps();
    auto data_apps_path = std::format("{}/apps", file::MOUNT_POINT_DATA);
    if (file::isDirectory(data_apps_path)) {
        registerInstalledApps(data_apps_path);
    }
    registerInstalledAppsFromSdCards();
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
    service::loader::startApp(app::boot::manifest.appId);

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
