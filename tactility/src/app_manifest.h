#pragma once

#include "core_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct _lv_obj_t lv_obj_t;
typedef void* App;

typedef enum {
    /** A desktop app sits at the root of the app stack managed by the Loader service */
    AppTypeDesktop,
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

typedef struct {
    /**
     * The identifier by which the app is launched by the system and other apps.
     */
    const char* id;

    /**
     * The user-readable name of the app. Used in UI.
     */
    const char* name;

    /**
     * Optional icon.
     */
    const char* _Nullable icon;

    /**
     * App type affects launch behaviour.
     */
    const AppType type;

    /**
     * Non-blocking method to call when app is started.
     */
    const AppOnStart _Nullable on_start;

    /**
     * Non-blocking method to call when app is stopped.
     */
    const AppOnStop _Nullable on_stop;

    /**
     * Non-blocking method to create the GUI
     */
    const AppOnShow _Nullable on_show;

    /**
     * Non-blocking method, called before gui is destroyed
     */
    const AppOnHide _Nullable on_hide;
} AppManifest;

#ifdef __cplusplus
}
#endif
