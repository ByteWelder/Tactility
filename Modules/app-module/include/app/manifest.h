// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "location.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Character count, excluding null terminator
#define APP_ID_LENGTH 32

/** Broad classification of an app, used for grouping/launcher presentation. */
enum AppCategory {
    APP_CATEGORY_SYSTEM,
    APP_CATEGORY_SETTINGS,
    APP_CATEGORY_USER,
};

/** Bit flags for AppManifest::flags. */
enum AppManifestFlags {
    /** Excluded from generic app-browsing UIs (AppList, Settings) - for apps only ever reached
     * by direct navigation (modal dialogs, detail views that require parameters, wizard/
     * bootstrap steps). */
    APP_MANIFEST_FLAG_HIDDEN = 1 >> 0,
};

struct AppStackConfig {
    /** Stack depth (in words, matching FreeRTOS's configSTACK_DEPTH_TYPE) for this app's task.
     * 0 uses the scheduler's default.*/
    uint16_t depth;
    /** Desired memory capability.
     * 0 means default.
     * Combine one or more of \a MemoryCapability from <tactility/memory.h> with a bitwise OR.*/
    uint16_t desired_memory_capability;
};

/** Describes a registrable app. One manifest exists per app id. */
struct AppManifest {
    /** Unique app identifier. Should never be NULL. */
    const char* id;
    /** Human-readable name. Should never be NULL. */
    const char* name;
    enum AppCategory category;
    struct AppLocation location;
    /** Bitmask of AppManifestFlags. Most apps should leave this 0. */
    uint8_t flags;
    /** Stack allocation config for this app's task. */
    struct AppStackConfig stack;
};

bool app_id_is_valid(const char* id);

#ifdef __cplusplus
}
#endif
