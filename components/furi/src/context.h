#pragma once

typedef struct {
    /** Contextual data related to the running app's instance
     *
     * The app can attach its data to this.
     * The lifecycle is determined by the on_start and on_stop methods in the AppManifest.
     * These manifest methods can optionally allocate/free data that is attached here.
     */
    void* data;
} Context;
