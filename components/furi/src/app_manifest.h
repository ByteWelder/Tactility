#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct _lv_obj_t lv_obj_t;

typedef enum {
    AppTypeSystem,
    AppTypeSettings,
    AppTypeUser
} AppType;

typedef void (*AppOnStart)(void _Nonnull* parameter);
typedef void (*AppOnStop)();
typedef void (*AppOnShow)(lv_obj_t* parent, void* context);

typedef struct {
    /**
     * The identifier by which the app is launched by the system and other apps.
     */
    const char* _Nonnull id;

    /**
     * The user-readable name of the app. Used in UI.
     */
    const char* _Nonnull name;

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
} AppManifest;

#ifdef __cplusplus
}
#endif
