#include "app_i.h"

#include <stdio.h>
#include <stdlib.h>

static AppFlags tt_app_get_flags_default(AppType type);

// region Alloc/free

App tt_app_alloc(const AppManifest* manifest, Bundle* _Nullable parameters) {
    AppData* data = malloc(sizeof(AppData));
    *data = (AppData) {
        .mutex = tt_mutex_alloc(MutexTypeRecursive),
        .state = APP_STATE_INITIAL,
        .flags = tt_app_get_flags_default(manifest->type),
        .manifest = manifest,
        .parameters = parameters,
        .data = NULL
    };
    return (App*)data;
}

void tt_app_free(App app) {
    AppData* data = (AppData*)app;
    if (data->parameters) {
        tt_bundle_free(data->parameters);
    }
    tt_mutex_free(data->mutex);
    free(data);
}

// endregion

// region Internal

static void tt_app_lock(AppData* data) {
    tt_mutex_acquire(data->mutex, MutexTypeRecursive);
}

static void tt_app_unlock(AppData* data) {
    tt_mutex_release(data->mutex);
}

static AppFlags tt_app_get_flags_default(AppType type) {
    static const AppFlags DEFAULT_DESKTOP_FLAGS = {
        .show_toolbar = false,
        .show_statusbar = true
    };

    static const AppFlags DEFAULT_APP_FLAGS = {
        .show_toolbar = true,
        .show_statusbar = true
    };

    return type == AppTypeDesktop
        ? DEFAULT_DESKTOP_FLAGS
        : DEFAULT_APP_FLAGS;
}

// endregion Internal

// region Public getters & setters

void tt_app_set_state(App app, AppState state) {
    AppData* data = (AppData*)app;
    tt_app_lock(data);
    data->state = state;
    tt_app_unlock(data);
}

AppState tt_app_get_state(App app) {
    AppData* data = (AppData*)app;
    tt_app_lock(data);
    AppState state = data->state;
    tt_app_unlock(data);
    return state;
}

const AppManifest* tt_app_get_manifest(App app) {
    AppData* data = (AppData*)app;
    // No need to lock const data;
    return data->manifest;
}

AppFlags tt_app_get_flags(App app) {
    AppData* data = (AppData*)app;
    tt_app_lock(data);
    AppFlags flags = data->flags;
    tt_app_unlock(data);
    return flags;
}

void tt_app_set_flags(App app, AppFlags flags) {
    AppData* data = (AppData*)app;
    tt_app_lock(data);
    data->flags = flags;
    tt_app_unlock(data);
}

void* tt_app_get_data(App app) {
    AppData* data = (AppData*)app;
    tt_app_lock(data);
    void* value = data->data;
    tt_app_unlock(data);
    return value;
}

void tt_app_set_data(App app, void* value) {
    AppData* data = (AppData*)app;
    tt_app_lock(data);
    data->data = value;
    tt_app_unlock(data);
}

/** TODO: Make this thread-safe.
 * In practice, the bundle is writeable, so someone could be writing to it
 * while it is being accessed from another thread.
 * Consider creating MutableBundle vs Bundle.
 * Consider not exposing bundle, but expose `app_get_bundle_int(key)` methods with locking in it.
 */
Bundle* _Nullable tt_app_get_parameters(App app) {
    AppData* data = (AppData*)app;
    tt_app_lock(data);
    Bundle* bundle = data->parameters;
    tt_app_unlock(data);
    return bundle;
}

// endregion Public getters & setters
