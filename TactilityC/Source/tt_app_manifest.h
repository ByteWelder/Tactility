#pragma once

#include "tt_bundle.h"
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AppResultOk,
    AppResultCancelled,
    AppResultError
} Result;

typedef void* AppContextHandle;

typedef void (*AppOnStart)(AppContextHandle app);
typedef void (*AppOnStop)(AppContextHandle app);
typedef void (*AppOnShow)(AppContextHandle app, lv_obj_t* parent);
typedef void (*AppOnHide)(AppContextHandle app);
typedef void (*AppOnResult)(AppContextHandle app, Result result, BundleHandle resultData);

/**
 * This is used to register the manifest of an external app.
 * @param[in] name the application's human-readable name
 * @param[in] icon the optional application icon (you can use LV_SYMBOL_* too)
 * @param[in] onStart called when the app is launched (started)
 * @param[in] onStop called when the app is exited (stopped)
 * @param[in] onShow called when the app is about to be shown to the user (app becomes visible)
 * @param[in] onHide called when the app is about to be invisible to the user (e.g. other app was launched by this app, and this app goes to the background)
 * @param[in] onResult called when the app receives a result after launching another app
 */
void tt_set_app_manifest(
    const char* name,
    const char* _Nullable icon,
    AppOnStart _Nullable onStart,
    AppOnStop _Nullable onStop,
    AppOnShow _Nullable onShow,
    AppOnHide _Nullable onHide,
    AppOnResult _Nullable onResult
);

#ifdef __cplusplus
}
#endif