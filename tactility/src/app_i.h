#pragma once

#include "app.h"

#include "app_manifest.h"
#include "Mutex.h"

namespace tt {

class AppData {
public:
    Mutex* mutex;
    const AppManifest* manifest;
    AppState state = AppStateInitial;
    /** @brief Memory marker at start of app, to detect memory leaks */
    size_t memory = 0;
    AppFlags flags = {
        .flags = 0
    };
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
};

} // namespace
