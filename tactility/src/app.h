#pragma once

#include "app_manifest.h"
#include "Bundle.h"
#include "Mutex.h"

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
    Bundle parameters;
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
App tt_app_alloc(const AppManifest& manifest, const Bundle& parameters);
void tt_app_free(App app);

void tt_app_set_state(App app, AppState state);
AppState tt_app_get_state(App app);

const AppManifest& tt_app_get_manifest(App app);

AppFlags tt_app_get_flags(App app);
void tt_app_set_flags(App app, AppFlags flags);

void* _Nullable tt_app_get_data(App app);
void tt_app_set_data(App app, void* data);

const Bundle& tt_app_get_parameters(App app);
