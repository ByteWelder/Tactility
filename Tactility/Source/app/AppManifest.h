#pragma once

#include "CoreDefines.h"
#include "ManifestRegistry.h"
#include <Bundle.h>
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
    Ok,
    Cancelled,
    Error
};

typedef App*(*CreateApp)();

struct AppManifest {
    /** The identifier by which the app is launched by the system and other apps. */
    std::string id;

    /** The user-readable name of the app. Used in UI. */
    std::string name;

    /** Optional icon. */
    std::string icon = {};

    /** App type affects launch behaviour. */
    Type type = Type::User;

    /** Create the instance of the app */
    CreateApp createApp = nullptr;
};

struct {
    bool operator()(const AppManifest* left, const AppManifest* right) const { return left->name < right->name; }
} SortAppManifestByName;

} // namespace
