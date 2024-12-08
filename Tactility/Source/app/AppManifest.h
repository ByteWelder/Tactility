#pragma once

#include <string>
#include <Bundle.h>
#include "CoreDefines.h"

// Forward declarations
typedef struct _lv_obj_t lv_obj_t;

namespace tt::app {

class AppContext;

typedef enum {
    /** Boot screen, shown before desktop is launched. */
    TypeBoot,
    /** A desktop app sits at the root of the app stack managed by the Loader service */
    TypeDesktop,
    /** Apps that generally aren't started from the desktop (e.g. image viewer) */
    TypeHidden,
    /** Standard apps, provided by the system. */
    TypeSystem,
    /** The apps that are launched/shown by the Settings app. The Settings app itself is of type AppTypeSystem. */
    TypeSettings,
    /** User-provided apps. */
    TypeUser
} Type;

typedef enum {
    ResultOk,
    ResultCancelled,
    ResultError
} Result;

typedef void (*AppOnStart)(AppContext& app);
typedef void (*AppOnStop)(AppContext& app);
typedef void (*AppOnShow)(AppContext& app, lv_obj_t* parent);
typedef void (*AppOnHide)(AppContext& app);
typedef void (*AppOnResult)(AppContext& app, Result result, const Bundle& resultData);

struct AppManifest {
    /**
     * The identifier by which the app is launched by the system and other apps.
     */
    std::string id;

    /**
     * The user-readable name of the app. Used in UI.
     */
    std::string name;

    /**
     * Optional icon.
     */
    std::string icon = {};

    /**
     * App type affects launch behaviour.
     */
    Type type = TypeUser;

    /**
     * Non-blocking method to call when app is started.
     */
    AppOnStart onStart = nullptr;

    /**
     * Non-blocking method to call when app is stopped.
     */
    AppOnStop _Nullable onStop = nullptr;

    /**
     * Non-blocking method to create the GUI
     */
    AppOnShow _Nullable onShow = nullptr;

    /**
     * Non-blocking method, called before gui is destroyed
     */
    AppOnHide _Nullable onHide = nullptr;

    /**
     * Handle the result for apps that are launched
     */
    AppOnResult _Nullable onResult = nullptr;
};

struct {
    bool operator()(const AppManifest* left, const AppManifest* right) const { return left->name < right->name; }
} SortAppManifestByName;

} // namespace
