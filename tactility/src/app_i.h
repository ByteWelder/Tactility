#pragma once

#include "app.h"

#include "app_manifest.h"
#include "mutex.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Mutex* mutex;
    const AppManifest* manifest;
    AppState state;
    /** @brief Memory marker at start of app, to detect memory leaks */
    size_t memory;
    AppFlags flags;
    /** @brief Optional parameters to start the app with
     * When these are stored in the app struct, the struct takes ownership.
     * Do not mutate after app creation.
     */
    Bundle* _Nullable parameters;
    /** @brief @brief Contextual data related to the running app's instance
     * The app can attach its data to this.
     * The lifecycle is determined by the on_start and on_stop methods in the AppManifest.
     * These manifest methods can optionally allocate/free data that is attached here.
     */
    void* _Nullable data;
} AppData;

#ifdef __cplusplus
}
#endif
