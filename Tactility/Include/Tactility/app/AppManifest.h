#pragma once

#include <Tactility/app/AppRegistration.h>

#include <string>

namespace tt::app {

class App;
class AppContext;

/** Application types */
enum class Category {
    /** Standard apps, provided by the system. */
    System,
    /** The apps that are launched/shown by the Settings app. The Settings app itself is of type AppTypeSystem. */
    Settings,
    /** User-provided apps. */
    User
};

/** Result status code for application result callback. */
enum class Result {
    Ok = 0U,
    Cancelled = 1U,
    Error = 2U
};

class Location {

    std::string path;
    Location() = default;
    explicit Location(const std::string& path) : path(path) {}

public:

    static Location internal() { return {}; }

    static Location external(const std::string& path) {
        return Location(path);
    }

    /** Internal apps are all apps that are part of the firmware release. */
    bool isInternal() const { return path.empty(); }

    /**
     * External apps are all apps that are not part of the firmware release.
     * e.g. an application on the sd card or one that is installed in /data
     */
    bool isExternal() const { return !path.empty(); }
    const std::string& getPath() const { return path; }
};

typedef std::shared_ptr<App>(*CreateApp)();

struct AppManifest {

    struct Flags {
        constexpr static uint32_t None = 0;
        /** Don't show the statusbar */
        constexpr static uint32_t HideStatusBar = 1 << 0;
        /** Hint to other systems to not show this app (e.g. in launcher or settings) */
        constexpr static uint32_t Hidden = 1 << 1;
    };

    /** The identifier by which the app is launched by the system and other apps. */
    std::string id = {};

    /** The user-readable name of the app. Used in UI. */
    std::string name = {};

    /** Optional icon. */
    std::string icon = {};

    /** App category helps with listing apps in Launcher, app list or settings apps. */
    Category category = Category::User;

    /** Where the app is located */
    Location location = Location::internal();

    /** Controls various settings */
    uint32_t flags = Flags::None;

    /** Create the instance of the app */
    CreateApp createApp = nullptr;
};

struct {
    bool operator()(const std::shared_ptr<AppManifest>& left, const std::shared_ptr<AppManifest>& right) const { return left->name < right->name; }
} SortAppManifestByName;

} // namespace
