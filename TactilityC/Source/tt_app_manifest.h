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

typedef void* AppHandle;

/** Important: These function types must map to t::app types exactly */
typedef void* (*AppCreateData)();
typedef void (*AppDestroyData)(void* data);
typedef void (*AppOnStart)(AppHandle app, void* _Nullable data);
typedef void (*AppOnStop)(AppHandle app, void* _Nullable data);
typedef void (*AppOnShow)(AppHandle app, void* _Nullable data, lv_obj_t* parent);
typedef void (*AppOnHide)(AppHandle app, void* _Nullable data);
typedef void (*AppOnResult)(AppHandle app, void* _Nullable data, Result result, BundleHandle resultData);

typedef struct {
    /** The application's human-readable name */
    const char* name;
    /** The application icon (you can use LV_SYMBOL_* too) */
    const char* _Nullable icon;
    /** The application can allocate data to re-use later (e.g. struct with state) */
    AppCreateData _Nullable createData;
    /** If createData is specified, this one must be specified too */
    AppDestroyData _Nullable destroyData;
    /** Called when the app is launched (started) */
    AppOnStart _Nullable onStart;
    /** Called when the app is exited (stopped) */
    AppOnStop _Nullable onStop;
    /** Called when the app is about to be shown to the user (app becomes visible) */
    AppOnShow _Nullable onShow;
    /** Called when the app is about to be invisible to the user (e.g. other app was launched by this app, and this app goes to the background) */
    AppOnHide _Nullable onHide;
    /** Called when the app receives a result after launching another app */
    AppOnResult _Nullable onResult;
} ExternalAppManifest;

/** This is used to register the manifest of an external app. */
void tt_app_register(const ExternalAppManifest* manifest);

#ifdef __cplusplus
}
#endif