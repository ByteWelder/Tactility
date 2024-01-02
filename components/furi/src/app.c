#include "app_i.h"
#include "furi_core.h"
#include "log.h"
#include "furi_string.h"

#define TAG "app"

App* furi_app_alloc(const AppManifest* _Nonnull manifest) {
    App app = {
        .manifest = manifest,
        .ep_thread_args = NULL
    };
    App* app_ptr = malloc(sizeof(App));
    return memcpy(app_ptr, &app, sizeof(App));
}

void furi_app_free(App* app) {
    furi_assert(app);

    if (app->ep_thread_args) {
        free(app->ep_thread_args);
        app->ep_thread_args = NULL;
    }

    free(app);
}
