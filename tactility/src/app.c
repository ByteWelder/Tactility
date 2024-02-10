#include "app_i.h"

#include "log.h"
#include <stdlib.h>

#define TAG "app"

static AppFlags tt_app_get_flags_default(AppType type);

static const AppFlags DEFAULT_FLAGS = {
    .show_statusbar = true
};

// region Alloc/free

App tt_app_alloc(const AppManifest* manifest, Bundle* _Nullable parameters) {
#ifdef ESP_PLATFORM
    size_t memory_before = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
#else
    size_t memory_before = 0;
#endif
    AppData* data = malloc(sizeof(AppData));
    *data = (AppData) {
        .mutex = tt_mutex_alloc(MutexTypeNormal),
        .state = AppStateInitial,
        .flags = tt_app_get_flags_default(manifest->type),
        .manifest = manifest,
        .parameters = parameters,
        .memory = memory_before,
        .data = NULL
    };
    return (App*)data;
}

void tt_app_free(App app) {
    AppData* data = (AppData*)app;

    size_t memory_before = data->memory;

    if (data->parameters) {
        tt_bundle_free(data->parameters);
    }
    tt_mutex_free(data->mutex);
    free(data);

#ifdef ESP_PLATFORM
    size_t memory_after = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
#else
    size_t memory_after = 0;
#endif

    if (memory_after < memory_before) {
        TT_LOG_W(TAG, "Potential memory leak: gained %u bytes after closing app", memory_before - memory_after);
        TT_LOG_W(TAG, "Note that WiFi service frees up memory asynchronously and that the leak can be caused by an app that was launched by this app.");
    }
}

// endregion

// region Internal

static void tt_app_lock(AppData* data) {
    tt_mutex_acquire(data->mutex, TtWaitForever);
}

static void tt_app_unlock(AppData* data) {
    tt_mutex_release(data->mutex);
}

static AppFlags tt_app_get_flags_default(AppType type) {
    return DEFAULT_FLAGS;
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
