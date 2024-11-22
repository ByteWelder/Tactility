#pragma once

#include "app_manifest.h"
#include "Bundle.h"
#include "Mutex.h"

namespace tt {

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


class AppInstance {
    Mutex mutex = Mutex(MutexTypeNormal);
    const AppManifest& manifest;
    AppState state = AppStateInitial;
    AppFlags flags = { .show_statusbar = true };
    /** @brief Optional parameters to start the app with
     * When these are stored in the app struct, the struct takes ownership.
     * Do not mutate after app creation.
     */
    tt::Bundle parameters;
    /** @brief @brief Contextual data related to the running app's instance
     * The app can attach its data to this.
     * The lifecycle is determined by the on_start and on_stop methods in the AppManifest.
     * These manifest methods can optionally allocate/free data that is attached here.
     */
    void* _Nullable data = nullptr;
public:
    AppInstance(const AppManifest& manifest) :
        manifest(manifest) {}

    AppInstance(const AppManifest& manifest, const Bundle& parameters) :
        manifest(manifest),
        parameters(parameters) {}

    void setState(AppState state);
    AppState getState();

    const AppManifest& getManifest();

    AppFlags getFlags();
    void setFlags(AppFlags flags);

    _Nullable void* getData();
    void setData(void* data);

    const Bundle& getParameters();
};

/** @brief Create an app
 * @param manifest
 * @param parameters optional bundle. memory ownership is transferred to App
 * @return
 */
[[deprecated("use class")]]
App tt_app_alloc(const AppManifest& manifest, const Bundle& parameters);
[[deprecated("use class")]]
void tt_app_free(App app);

[[deprecated("use class")]]
void tt_app_set_state(App app, AppState state);
[[deprecated("use class")]]
AppState tt_app_get_state(App app);

[[deprecated("use class")]]
const AppManifest& tt_app_get_manifest(App app);

[[deprecated("use class")]]
AppFlags tt_app_get_flags(App app);
[[deprecated("use class")]]
void tt_app_set_flags(App app, AppFlags flags);

[[deprecated("use class")]]
void* _Nullable tt_app_get_data(App app);
[[deprecated("use class")]]
void tt_app_set_data(App app, void* data);

[[deprecated("use class")]]
const Bundle& tt_app_get_parameters(App app);

} // namespace
