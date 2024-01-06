#pragma once

#include "app_manifest.h"
#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const AppManifest* manifest;
    Context context;
} App;

App* furi_app_alloc(const AppManifest* _Nonnull manifest);
void furi_app_free(App* _Nonnull app);

#ifdef __cplusplus
}
#endif
