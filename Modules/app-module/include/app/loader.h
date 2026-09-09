// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "location.h"
#include <app/manifest.h>
#include <stdbool.h>
#include <stdint.h>
#include <tactility/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/** service-module id the AppLoaderApi implementation for AppManifest::location.type ==
 * APP_LOCATION_MEMORY must register under. Implemented by app-module itself (source/app_internal_loader.cpp). */
#define APP_LOADER_MEMORY_SERVICE_ID "app-loader-memory"

/** service-module id the AppLoaderApi implementation for AppManifest::location.type ==
 * APP_LOCATION_PATH must register under. Implemented by a platform module (e.g. app-esp32-module). */
#define APP_LOADER_PATH_SERVICE_ID "app-loader-path"

/** Entry point signature for an APP_LOCATION_MEMORY app.
 * AppManifest::location.location holds this cast to void*. */
typedef int32_t (*AppMainFn)(int argc, char* argv[]);

typedef void* AppRuntime;

/**
 * Pluggable mechanism for loading and executing an app.
 */
struct AppLoaderApi {
    /**
     * Prepares an app instance for execution (e.g. read + relocate its binary).
     * @param[in] location the location to load the elf from
     * @param[out] out_runtime opaque handle to whatever load() allocated; passed back to run()/unload()
     */
    error_t (*load)(struct AppLocation location, AppRuntime* out_runtime);

    /**
     * Blocking: runs the app to completion.
     * @param[in] runtime handle produced by load()
     * @param[in] app_instance_id the running instance's id
     * @param[in] argc the amount of arguments in @a argv
     * @param[in] argv the array of string pointers (can be NULL)
     */
    int32_t (*run)(AppRuntime runtime, uint32_t app_instance_id, int argc, char* argv[]);

    /** Releases whatever load() allocated. Called after run() returns. */
    void (*unload)(AppRuntime runtime);

    /**
     * Reports whether this loader could load and run whatever @a location points at, without actually loading it.
     */
    bool (*is_executable)(struct AppLocation location);
};

#ifdef __cplusplus
}
#endif
