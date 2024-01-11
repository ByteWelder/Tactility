#pragma once

#include "context.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct _lv_obj_t lv_obj_t;

typedef enum {
    AppTypeDesktop,
    AppTypeSystem,
    AppTypeSettings,
    AppTypeUser
} AppType;

typedef void (*AppOnStart)(Context* context);
typedef void (*AppOnStop)(Context* context);
typedef void (*AppOnShow)(Context* context, lv_obj_t* parent);
typedef void (*AppOnHide)(Context* context);

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
