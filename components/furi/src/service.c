#include "service_i.h"
#include "furi_core.h"
#include "log.h"

#define TAG "service"

Service* furi_service_alloc(const ServiceManifest* _Nonnull manifest) {
    Service app = {
        .manifest = manifest,
        .context = {
            .data = NULL
        }
    };
    Service* app_ptr = malloc(sizeof(Service));
    return memcpy(app_ptr, &app, sizeof(Service));
}

void furi_service_free(Service* app) {
    furi_assert(app);
    free(app);
}
