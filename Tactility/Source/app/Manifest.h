#pragma once

#include <string>
#include <Bundle.h>
#include "CoreDefines.h"

// Forward declarations
typedef struct _lv_obj_t lv_obj_t;

namespace tt::app {

class App;

typedef enum {
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

typedef void (*AppOnStart)(App& app);
typedef void (*AppOnStop)(App& app);
typedef void (*AppOnShow)(App& app, lv_obj_t* parent);
typedef void (*AppOnHide)(App& app);
typedef void (*AppOnResult)(App& app, Result result, const Bundle& resultData);

typedef struct Manifest {
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
    const Type type = TypeUser;

    /**
     * Non-blocking method to call when app is started.
     */
    const AppOnStart onStart = nullptr;

    /**
     * Non-blocking method to call when app is stopped.
     */
    const AppOnStop _Nullable onStop = nullptr;

    /**
     * Non-blocking method to create the GUI
     */
    const AppOnShow _Nullable onShow = nullptr;

    /**
     * Non-blocking method, called before gui is destroyed
     */
    const AppOnHide _Nullable onHide = nullptr;

    /**
     * Handle the result for apps that are launched
     */
    const AppOnResult _Nullable onResult = nullptr;
} Manifest;

struct {
    bool operator()(const Manifest* left, const Manifest* right) const { return left->name < right->name; }
} SortAppManifestByName;

} // namespace
