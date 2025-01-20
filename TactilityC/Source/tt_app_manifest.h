#pragma once

#include "tt_bundle.h"
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Important: These values must map to tt::app::Result values exactly */
typedef enum {
    AppResultOk = 0,
    AppResultCancelled = 1,
    AppResultError = 2
} Result;

typedef void* AppContextHandle;

/** Important: These function types must map to t::app types exactly */
typedef void* (*AppCreateData)();
typedef void (*AppDestroyData)(void* data);
typedef void (*AppOnStart)(AppContextHandle app, void* _Nullable data);
typedef void (*AppOnStop)(AppContextHandle app, void* _Nullable data);
typedef void (*AppOnShow)(AppContextHandle app, void* _Nullable data, lv_obj_t* parent);
typedef void (*AppOnHide)(AppContextHandle app, void* _Nullable data);
typedef void (*AppOnResult)(AppContextHandle app, void* _Nullable data, Result result, BundleHandle resultData);

/**
 * This is used to register the manifest of an external app.
 * @param[in] name the application's human-readable name
 * @param[in] icon the application icon (you can use LV_SYMBOL_* too)
 * @param[in] createData the application can allocate data to re-use later (e.g. struct with state)
 * @param[in] destroyData if createData is specified, this one must be specified too
 * @param[in] onStart called when the app is launched (started)
 * @param[in] onStart called when the app is launched (started)
 * @param[in] onStop called when the app is exited (stopped)
 * @param[in] onShow called when the app is about to be shown to the user (app becomes visible)
 * @param[in] onHide called when the app is about to be invisible to the user (e.g. other app was launched by this app, and this app goes to the background)
 * @param[in] onResult called when the app receives a result after launching another app
 */
void tt_set_app_manifest(
    const char* name,
    const char* _Nullable icon,
    AppCreateData _Nullable createData,
    AppDestroyData _Nullable destroyData,
    AppOnStart _Nullable onStart,
    AppOnStop _Nullable onStop,
    AppOnShow _Nullable onShow,
    AppOnHide _Nullable onHide,
    AppOnResult _Nullable onResult
);

#ifdef __cplusplus
}
#endif