#include "app_i.h"
#include "furi_core.h"
#include "log.h"
#include "furi_string.h"

#define TAG "app"

const char* prv_type_service = "service";
const char* prv_type_system = "system";
const char* prv_type_user = "user";

static FuriThreadPriority get_thread_priority(AppType type) {
    switch (type) {
        case AppTypeService:
            return FuriThreadPriorityHighest;
        case AppTypeSystem:
            return FuriThreadPriorityHigh;
        case AppTypeUser:
            return FuriThreadPriorityNormal;
        default:
            furi_crash("no priority defined for app type");
    }
}

const char* furi_app_type_to_string(AppType type) {
    switch (type) {
        case AppTypeService:
            return prv_type_service;
        case AppTypeSystem:
            return prv_type_system;
        case AppTypeUser:
            return prv_type_user;
        default:
            furi_crash();
    }
}

App* furi_app_alloc(const AppManifest* _Nonnull manifest) {
    App app = {
        .manifest = manifest,
        .thread = NULL,
        .ep_thread_args = NULL
    };
    App* app_ptr = malloc(sizeof(App));
    return memcpy(app_ptr, &app, sizeof(App));
}

void furi_app_free(App* app) {
    furi_assert(app);

    if(app->thread) {
        furi_thread_join(app->thread);
        furi_thread_free(app->thread);
    }

    if (app->ep_thread_args) {
        free(app->ep_thread_args);
        app->ep_thread_args = NULL;
    }

    free(app);
}

FuriThread* furi_app_alloc_thread(App _Nonnull* app, const char* args) {
    FURI_LOG_I(
        TAG,
        "Starting %s app \"%s\"",
        furi_app_type_to_string(app->manifest->type),
        app->manifest->name
    );

    // Free any previous app launching arguments
    if (app->ep_thread_args) {
        free(app->ep_thread_args);
    }

    if (args) {
        app->ep_thread_args = strdup(args);
    } else {
        app->ep_thread_args = NULL;
    }

    FuriThread* thread = furi_thread_alloc_ex(
        app->manifest->name,
        app->manifest->stack_size,
        app->manifest->entry_point,
        app
    );

    if (app->manifest->type == AppTypeService) {
        furi_thread_mark_as_service(thread);
    }

    FuriString* app_name = furi_string_alloc();
    furi_thread_set_appid(thread, furi_string_get_cstr(app_name));
    furi_string_free(app_name);

    FuriThreadPriority priority = get_thread_priority(app->manifest->type);
    furi_thread_set_priority(thread, priority);

    return furi_thread_get_id(thread);
}
