#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#include <Tactility/Tactility.h>
#include <Tactility/TactilityConfig.h>

#include <Tactility/app/AppManifestParsing.h>
#include <Tactility/app/AppRegistration.h>
#include <Tactility/DispatcherThread.h>
#include <Tactility/file/File.h>
#include <Tactility/file/FileLock.h>
#include <Tactility/file/PropertiesFile.h>
#include <Tactility/hal/HalPrivate.h>
#include <Tactility/lvgl/LvglPrivate.h>
#include <Tactility/MountPoints.h>
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
#endif
#ifdef CONFIG_ESP_WIFI_ENABLED
    namespace espnow { extern const ServiceManifest manifest; }
#endif
    // Secondary (UI)
    namespace gui { extern const ServiceManifest manifest; }
    namespace loader { extern const ServiceManifest manifest; }
    namespace memorychecker { extern const ServiceManifest manifest; }
    namespace statusbar { extern const ServiceManifest manifest; }
#if TT_FEATURE_SCREENSHOT_ENABLED
    namespace screenshot { extern const ServiceManifest manifest; }
#endif

}

// endregion

// region Default apps

namespace app {
    namespace addgps { extern const AppManifest manifest; }
    namespace apphub { extern const AppManifest manifest; }
    namespace apphubdetails { extern const AppManifest manifest; }
    namespace alertdialog { extern const AppManifest manifest; }
    namespace appdetails { extern const AppManifest manifest; }
    namespace applist { extern const AppManifest manifest; }
    namespace appsettings { extern const AppManifest manifest; }
    namespace boot { extern const AppManifest manifest; }
#ifdef CONFIG_ESP_WIFI_ENABLED
    namespace chat { extern const AppManifest manifest; }
#endif
    namespace development { extern const AppManifest manifest; }
    namespace display { extern const AppManifest manifest; }
    namespace files { extern const AppManifest manifest; }
    namespace fileselection { extern const AppManifest manifest; }
    namespace gpssettings { extern const AppManifest manifest; }
    namespace i2cscanner { extern const AppManifest manifest; }
    namespace i2csettings { extern const AppManifest manifest; }
    namespace imageviewer { extern const AppManifest manifest; }
    namespace inputdialog { extern const AppManifest manifest; }
    namespace launcher { extern const AppManifest manifest; }
    namespace localesettings { extern const AppManifest manifest; }
    namespace notes { extern const AppManifest manifest; }
    namespace power { extern const AppManifest manifest; }
    namespace selectiondialog { extern const AppManifest manifest; }
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
static void registerInternalApps() {
    TT_LOG_I(TAG, "Registering internal apps");

    addAppManifest(app::alertdialog::manifest);
    addAppManifest(app::appdetails::manifest);
    addAppManifest(app::apphub::manifest);
    addAppManifest(app::apphubdetails::manifest);
    addAppManifest(app::applist::manifest);
    addAppManifest(app::appsettings::manifest);
    addAppManifest(app::display::manifest);
    addAppManifest(app::files::manifest);
    addAppManifest(app::fileselection::manifest);
    addAppManifest(app::imageviewer::manifest);
    addAppManifest(app::inputdialog::manifest);
    addAppManifest(app::launcher::manifest);
    addAppManifest(app::localesettings::manifest);
    addAppManifest(app::notes::manifest);
    addAppManifest(app::settings::manifest);
    addAppManifest(app::selectiondialog::manifest);
    addAppManifest(app::systeminfo::manifest);
    addAppManifest(app::timedatesettings::manifest);
    addAppManifest(app::timezone::manifest);
    addAppManifest(app::wifiapsettings::manifest);
    addAppManifest(app::wificonnect::manifest);
    addAppManifest(app::wifimanage::manifest);

#if defined(CONFIG_TINYUSB_MSC_ENABLED) && CONFIG_TINYUSB_MSC_ENABLED
    addAppManifest(app::usbsettings::manifest);
#endif

#if TT_FEATURE_SCREENSHOT_ENABLED
    addAppManifest(app::screenshot::manifest);
#endif

#ifdef CONFIG_ESP_WIFI_ENABLED
    addAppManifest(app::chat::manifest);
#endif

#ifdef ESP_PLATFORM
    addAppManifest(app::crashdiagnostics::manifest);
    addAppManifest(app::development::manifest);
#endif

    if (!hal::getConfiguration()->i2c.empty()) {
        addAppManifest(app::i2cscanner::manifest);
        addAppManifest(app::i2csettings::manifest);
    }

    if (!hal::getConfiguration()->uart.empty()) {
        addAppManifest(app::addgps::manifest);
        addAppManifest(app::gpssettings::manifest);
    }

    if (hal::hasDevice(hal::Device::Type::Power)) {
        addAppManifest(app::power::manifest);
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

    app::addAppManifest(manifest);
}

static void registerInstalledApps(const std::string& path) {
    TT_LOG_I(TAG, "Registering apps from %s", path.c_str());

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
    if (file::isDirectory(app_path)) {
        registerInstalledApps(app_path);
    }
}

static void registerInstalledAppsFromSdCards() {
    auto sdcard_devices = hal::findDevices<hal::sdcard::SdCardDevice>(hal::Device::Type::SdCard);
    for (const auto& sdcard : sdcard_devices) {
        if (sdcard->isMounted()) {
            TT_LOG_I(TAG, "Registering apps from %s", sdcard->getMountPath().c_str());
            registerInstalledAppsFromSdCard(sdcard);
        }
    }
}

static void registerAndStartSecondaryServices() {
    TT_LOG_I(TAG, "Registering and starting system services");
    addService(service::loader::manifest);
    addService(service::gui::manifest);
    addService(service::statusbar::manifest);
    addService(service::memorychecker::manifest);
#if TT_FEATURE_SCREENSHOT_ENABLED
    addService(service::screenshot::manifest);
#endif
}

static void registerAndStartPrimaryServices() {
    TT_LOG_I(TAG, "Registering and starting system services");
    addService(service::gps::manifest);
    if (hal::hasDevice(hal::Device::Type::SdCard)) {
        addService(service::sdcard::manifest);
    }
    addService(service::wifi::manifest);
#ifdef ESP_PLATFORM
    addService(service::development::manifest);
#endif

#ifdef CONFIG_ESP_WIFI_ENABLED
    addService(service::espnow::manifest);
#endif
}

void createTempDirectory(const std::string& rootPath) {
    auto temp_path = std::format("{}/tmp", rootPath);
    if (!file::isDirectory(temp_path)) {
        auto lock = file::getLock(rootPath)->asScopedLock();
        if (lock.lock(1000 / portTICK_PERIOD_MS)) {
            if (mkdir(temp_path.c_str(), 0777) == 0) {
                TT_LOG_I(TAG, "Created %s", temp_path.c_str());
            } else {
                TT_LOG_E(TAG, "Failed to create %s", temp_path.c_str());
            }
        } else {
            TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, rootPath.c_str());
        }
    } else {
        TT_LOG_I(TAG, "Found existing %s", temp_path.c_str());
    }
}

void prepareFileSystems() {
    // Temporary directories for SD cards
    auto sdcard_devices = hal::findDevices<hal::sdcard::SdCardDevice>(hal::Device::Type::SdCard);
    for (const auto& sdcard : sdcard_devices) {
        if (sdcard->isMounted()) {
            createTempDirectory(sdcard->getMountPath());
        }
    }
    // Temporary directory for /data
    if (file::isDirectory(file::MOUNT_POINT_DATA)) {
        createTempDirectory(file::MOUNT_POINT_DATA);
    }
}

void registerApps() {
    registerInternalApps();
    auto data_apps_path = std::format("{}/apps", file::MOUNT_POINT_DATA);
    if (file::isDirectory(data_apps_path)) {
        registerInstalledApps(data_apps_path);
    }
    registerInstalledAppsFromSdCards();
}

void run(const Configuration& config) {
    TT_LOG_I(TAG, "Tactility v%s on %s (%s)", TT_VERSION, CONFIG_TT_DEVICE_NAME, CONFIG_TT_DEVICE_ID);

    assert(config.hardware);
    const hal::Configuration& hardware = *config.hardware;

    // Assign early so starting services can use it
    config_instance = &config;

#ifdef ESP_PLATFORM
    initEsp();
#endif
    file::setFindLockFunction(file::findLock);
    settings::initTimeZone();
    hal::init(*config.hardware);
    network::ntp::init();

    registerAndStartPrimaryServices();
    lvgl::init(hardware);
    registerAndStartSecondaryServices();

    TT_LOG_I(TAG, "Core systems ready");

    TT_LOG_I(TAG, "Starting boot app");
    // The boot app takes care of registering system apps, user services and user apps
    addAppManifest(app::boot::manifest);
    app::start(app::boot::manifest.appId);

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
