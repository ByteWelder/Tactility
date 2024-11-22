#pragma once

#include <string>
#include "CoreDefines.h"

// Forward declarations
typedef struct _lv_obj_t lv_obj_t;

namespace tt {

typedef void* App;

typedef enum {
    /** A desktop app sits at the root of the app stack managed by the Loader service */
    AppTypeDesktop,
    /** Apps that generally aren't started from the desktop (e.g. image viewer) */
    AppTypeHidden,
    /** Standard apps, provided by the system. */
    AppTypeSystem,
    /** The apps that are launched/shown by the Settings app. The Settings app itself is of type AppTypeSystem. */
    AppTypeSettings,
    /** User-provided apps. */
    AppTypeUser
} AppType;

typedef void (*AppOnStart)(App app);
typedef void (*AppOnStop)(App app);
typedef void (*AppOnShow)(App app, lv_obj_t* parent);
typedef void (*AppOnHide)(App app);

typedef struct AppManifest {
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
    const AppType type = AppTypeUser;

    /**
     * Non-blocking method to call when app is started.
     */
    const AppOnStart on_start = nullptr;

    /**
     * Non-blocking method to call when app is stopped.
     */
    const AppOnStop _Nullable on_stop = nullptr;

    /**
     * Non-blocking method to create the GUI
     */
    const AppOnShow _Nullable on_show = nullptr;

    /**
     * Non-blocking method, called before gui is destroyed
     */
    const AppOnHide _Nullable on_hide = nullptr;
} AppManifest;

} // namespace
