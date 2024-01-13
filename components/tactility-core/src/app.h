#pragma once

#include "app_manifest.h"
#include "bundle.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_STATE_INITIAL, // App is being activated in loader
    APP_STATE_STARTED, // App is in memory
    APP_STATE_SHOWING, // App view is created
    APP_STATE_HIDING,  // App view is destroyed
    APP_STATE_STOPPED  // App is not in memory
} AppState;

typedef union {
    struct {
        bool show_statusbar : 1;
        bool show_toolbar : 1;
    };
    unsigned char flags;
} AppFlags;

typedef void* App;

/** @brief Create an app
 * @param manifest
 * @param parameters optional bundle. memory ownership is transferred to App
 * @return
 */
App app_alloc(const AppManifest* manifest, Bundle* _Nullable parameters);
void app_free(App app);

void app_set_state(App app, AppState state);
AppState app_get_state(App app);

const AppManifest* app_get_manifest(App app);

AppFlags app_get_flags(App app);
void app_set_flags(App app, AppFlags flags);

void* _Nullable app_get_data(App app);
void app_set_data(App app, void* data);

Bundle* _Nullable app_get_parameters(App app);

#ifdef __cplusplus
}
#endif
