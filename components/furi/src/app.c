#include "app_i.h"

#include <stdio.h>

static AppFlags app_get_flags_default(AppType type);

// region Alloc/free

App app_alloc(const AppManifest* manifest, Bundle* _Nullable bundle) {
    AppData* data = malloc(sizeof(AppData));
    *data = (AppData) {
        .mutex = furi_mutex_alloc(FuriMutexTypeRecursive),
        .state = APP_STATE_INITIAL,
        .flags = app_get_flags_default(manifest->type),
        .manifest = manifest,
        .bundle = bundle,
        .data = NULL
    };
    return (App*)data;
}

void app_free(App app) {
    AppData* data = (AppData*)app;
    furi_mutex_free(data->mutex);
    free(data);
}

// endregion

// region Internal

static void app_lock(AppData* data) {
    furi_mutex_acquire(data->mutex, FuriMutexTypeRecursive);
}

static void app_unlock(AppData* data) {
    furi_mutex_release(data->mutex);
}

static AppFlags app_get_flags_default(AppType type) {
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

void app_set_state(App app, AppState state) {
    AppData* data = (AppData*)app;
    app_lock(data);
    data->state = state;
    app_unlock(data);
}

AppState app_get_state(App app) {
    AppData* data = (AppData*)app;
    app_lock(data);
    AppState state = data->state;
    app_unlock(data);
    return state;
}

const AppManifest* app_get_manifest(App app) {
    AppData* data = (AppData*)app;
    // No need to lock const data;
    return data->manifest;
}

AppFlags app_get_flags(App app) {
    AppData* data = (AppData*)app;
    app_lock(data);
    AppFlags flags = data->flags;
    app_unlock(data);
    return flags;
}

void app_set_flags(App app, AppFlags flags) {
    AppData* data = (AppData*)app;
    app_lock(data);
    data->flags = flags;
    app_unlock(data);
}

void* app_get_data(App app) {
    AppData* data = (AppData*)app;
    app_lock(data);
    void* value = data->data;
    app_unlock(data);
    return value;
}

void app_set_data(App app, void* value) {
    AppData* data = (AppData*)app;
    app_lock(data);
    data->data = value;
    app_unlock(data);
}

/** TODO: Make this thread-safe.
 * In practice, the bundle is writeable, so someone could be writing to it
 * while it is being accessed from another thread.
 * Consider creating MutableBundle vs Bundle.
 * Consider not exposing bundle, but expose `app_get_bundle_int(key)` methods with locking in it.
 */
Bundle* _Nullable app_get_bundle(App app) {
    AppData* data = (AppData*)app;
    app_lock(data);
    Bundle* bundle = data->bundle;
    app_unlock(data);
    return bundle;
}

// endregion Public getters & setters
