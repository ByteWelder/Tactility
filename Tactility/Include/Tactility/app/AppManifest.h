#pragma once

#include "Tactility/app/ManifestRegistry.h"

#include <Tactility/CoreDefines.h>
#include <Tactility/Bundle.h>

#include <string>

// Forward declarations
typedef struct _lv_obj_t lv_obj_t;

namespace tt::app {

class App;
class AppContext;

/** Application types */
enum class Type {
    /** Boot screen, shown before desktop is launched. */
    Boot,
    /** A launcher app sits at the root of the app stack after the boot splash is finished */
    Launcher,
    /** Apps that generally aren't started from the desktop (e.g. image viewer) */
    Hidden,
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

private:

    std::string path;
    Location() = default;
    explicit Location(const std::string& path) : path(path) {}

public:

    static Location internal() { return {}; }

    static Location external(const std::string& path) {
        return Location(path);
    }

    bool isInternal() const { return path.empty(); }
    bool isExternal() const { return !path.empty(); }
    const std::string& getPath() const { return path; }
};

typedef std::shared_ptr<App>(*CreateApp)();

struct AppManifest {
    /** The identifier by which the app is launched by the system and other apps. */
    std::string id = {};

    /** The user-readable name of the app. Used in UI. */
    std::string name = {};

    /** Optional icon. */
    std::string icon = {};

    /** App type affects launch behaviour. */
    Type type = Type::User;

    /** Where the app is located */
    Location location = Location::internal();

    /** Create the instance of the app */
    CreateApp createApp = nullptr;
};

struct {
    bool operator()(const std::shared_ptr<AppManifest>& left, const std::shared_ptr<AppManifest>& right) const { return left->name < right->name; }
} SortAppManifestByName;

} // namespace
