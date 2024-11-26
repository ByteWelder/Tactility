#pragma once

#include "Manifest.h"
#include "Bundle.h"
#include "Mutex.h"

namespace tt::app {

typedef enum {
    StateInitial, // App is being activated in loader
    StateStarted, // App is in memory
    StateShowing, // App view is created
    StateHiding,  // App view is destroyed
    StateStopped  // App is not in memory
} State;

typedef union {
    struct {
        bool show_statusbar : 1;
    };
    unsigned char flags;
} Flags;


class AppInstance {

private:

    Mutex mutex = Mutex(MutexTypeNormal);
    const Manifest& manifest;
    State state = StateInitial;
    Flags flags = { .show_statusbar = true };
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

    AppInstance(const Manifest& manifest) :
        manifest(manifest) {}

    AppInstance(const Manifest& manifest, const Bundle& parameters) :
        manifest(manifest),
        parameters(parameters) {}

    void setState(State state);
    State getState();

    const Manifest& getManifest();

    Flags getFlags();
    void setFlags(Flags flags);

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
App tt_app_alloc(const Manifest& manifest, const Bundle& parameters);
[[deprecated("use class")]]
void tt_app_free(App app);

[[deprecated("use class")]]
void tt_app_set_state(App app, State state);
[[deprecated("use class")]]
State tt_app_get_state(App app);

[[deprecated("use class")]]
const Manifest& tt_app_get_manifest(App app);

[[deprecated("use class")]]
Flags tt_app_get_flags(App app);
[[deprecated("use class")]]
void tt_app_set_flags(App app, Flags flags);

[[deprecated("use class")]]
void* _Nullable tt_app_get_data(App app);
[[deprecated("use class")]]
void tt_app_set_data(App app, void* data);

[[deprecated("use class")]]
const Bundle& tt_app_get_parameters(App app);

} // namespace
