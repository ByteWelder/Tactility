#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* AppContextHandle;
typedef void* BundleHandle;

typedef enum {
    AppResultOk,
    AppResultCancelled,
    AppResultError
} Result;

typedef void (*AppOnStart)(AppContextHandle app);
typedef void (*AppOnStop)(AppContextHandle app);
typedef void (*AppOnShow)(AppContextHandle app, lv_obj_t* parent);
typedef void (*AppOnHide)(AppContextHandle app);
typedef void (*AppOnResult)(AppContextHandle app, Result result, BundleHandle resultData);

void tt_set_app_manifest(
    const char* name,
    const char* _Nullable icon,
    AppOnStart onStart,
    AppOnStop _Nullable onStop,
    AppOnShow _Nullable onShow,
    AppOnHide _Nullable onHide,
    AppOnResult _Nullable onResult
);

#ifdef __cplusplus
}
#endif