#pragma once

#include "app_manifest.h"
#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    FuriThread* thread;
    const AppManifest* manifest;
    void* ep_thread_args;
} App;

const char* furi_app_type_to_string(AppType type);
FuriThread* furi_app_alloc_thread(App* _Nonnull app, const char* args);
App* furi_app_alloc(const AppManifest* _Nonnull manifest);
void furi_app_free(App* _Nonnull app);

#ifdef __cplusplus
}
#endif
