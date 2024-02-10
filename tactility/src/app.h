#pragma once

#include "app_manifest.h"
#include "bundle.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AppStateInitial, // App is being activated in loader
    AppStateStarted, // App is in memory
    AppStateShowing, // App view is created
    AppStateHiding,  // App view is destroyed
    AppStateStopped  // App is not in memory
} AppState;

typedef union {
    struct {
        bool show_statusbar : 1;
    };
    unsigned char flags;
} AppFlags;

typedef void* App;

/** @brief Create an app
 * @param manifest
 * @param parameters optional bundle. memory ownership is transferred to App
 * @return
 */
App tt_app_alloc(const AppManifest* manifest, Bundle* _Nullable parameters);
void tt_app_free(App app);

void tt_app_set_state(App app, AppState state);
AppState tt_app_get_state(App app);

const AppManifest* tt_app_get_manifest(App app);

AppFlags tt_app_get_flags(App app);
void tt_app_set_flags(App app, AppFlags flags);

void* _Nullable tt_app_get_data(App app);
void tt_app_set_data(App app, void* data);

Bundle* _Nullable tt_app_get_parameters(App app);

#ifdef __cplusplus
}
#endif
