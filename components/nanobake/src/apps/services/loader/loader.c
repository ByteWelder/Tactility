#include "loader.h"
#include "app_i.h"
#include "app_manifest.h"
#include "app_manifest_registry.h"
#include "loader_i.h"
#include <sys/cdefs.h>
#include "esp_heap_caps.h"

#define TAG "Loader"
#define LOADER_MAGIC_THREAD_VALUE 0xDEADBEEF

LoaderStatus loader_start(Loader* loader, const char* id, const char* args, FuriString* error_message) {
    LoaderMessage message;
    LoaderMessageLoaderStatusResult result;

    message.type = LoaderMessageTypeStartByName;
    message.start.id = id;
    message.start.args = args;
    message.start.error_message = error_message;
    message.api_lock = api_lock_alloc_locked();
    message.status_value = &result;
    furi_message_queue_put(loader->queue, &message, FuriWaitForever);
    api_lock_wait_unlock_and_free(message.api_lock);
    return result.value;
}

bool loader_lock(Loader* loader) {
    LoaderMessage message;
    LoaderMessageBoolResult result;
    message.type = LoaderMessageTypeLock;
    message.api_lock = api_lock_alloc_locked();
    message.bool_value = &result;
    furi_message_queue_put(loader->queue, &message, FuriWaitForever);
    api_lock_wait_unlock_and_free(message.api_lock);
    return result.value;
}

void loader_unlock(Loader* loader) {
    LoaderMessage message;
    message.type = LoaderMessageTypeUnlock;
    furi_message_queue_put(loader->queue, &message, FuriWaitForever);
}

bool loader_is_locked(Loader* loader) {
    LoaderMessage message;
    LoaderMessageBoolResult result;
    message.type = LoaderMessageTypeIsLocked;
    message.api_lock = api_lock_alloc_locked();
    message.bool_value = &result;
    furi_message_queue_put(loader->queue, &message, FuriWaitForever);
    api_lock_wait_unlock_and_free(message.api_lock);
    return result.value;
}

FuriPubSub* loader_get_pubsub(Loader* loader) {
    furi_assert(loader);
    // it's safe to return pubsub without locking
    // because it's never freed and loader is never exited
    // also the loader instance cannot be obtained until the pubsub is created
    return loader->pubsub;
}

// callbacks

static void loader_thread_state_callback(FuriThreadState thread_state, void* context) {
    furi_assert(context);

    Loader* loader = context;

    if (thread_state == FuriThreadStateRunning) {
        LoaderEvent event;
        event.type = LoaderEventTypeApplicationStarted;
        furi_pubsub_publish(loader->pubsub, &event);
    } else if (thread_state == FuriThreadStateStopped) {
        LoaderMessage message;
        message.type = LoaderMessageTypeAppClosed;
        furi_message_queue_put(loader->queue, &message, FuriWaitForever);
    }
}

// implementation

static Loader* loader_alloc() {
    Loader* loader = malloc(sizeof(Loader));
    loader->pubsub = furi_pubsub_alloc();
    loader->queue = furi_message_queue_alloc(1, sizeof(LoaderMessage));
    loader->app_data.args = NULL;
    loader->app_data.thread = NULL;
    loader->app_data.app = NULL;
    return loader;
}

static void loader_start_app_thread(Loader* loader) {
    // setup thread state callbacks
    furi_thread_set_state_context(loader->app_data.thread, loader);
    furi_thread_set_state_callback(loader->app_data.thread, loader_thread_state_callback);

    // start app thread
    furi_thread_start(loader->app_data.thread);
}

static void loader_log_status_error(
    LoaderStatus status,
    FuriString* error_message,
    const char* format,
    va_list args
) {
    if (error_message) {
        furi_string_vprintf(error_message, format, args);
        FURI_LOG_E(TAG, "Status [%d]: %s", status, furi_string_get_cstr(error_message));
    } else {
        FURI_LOG_E(TAG, "Status [%d]", status);
    }
}

static LoaderStatus loader_make_status_error(
    LoaderStatus status,
    FuriString* error_message,
    const char* format,
    ...
) {
    va_list args;
    va_start(args, format);
    loader_log_status_error(status, error_message, format, args);
    va_end(args);
    return status;
}

static LoaderStatus loader_make_success_status(FuriString* error_message) {
    if (error_message) {
        furi_string_set(error_message, "App started");
    }

    return LoaderStatusOk;
}

static void loader_start_app(
    Loader* loader,
    const AppManifest* _Nonnull manifest,
    const char* args
) {
    FURI_LOG_I(TAG, "Starting %s", manifest->id);

    App* _Nonnull app = furi_app_alloc(manifest);
    loader->app_data.app = app;

    FuriThread* thread = furi_app_alloc_thread(loader->app_data.app, args);
    loader->app_data.app->thread = thread;
    loader->app_data.thread = thread;

    loader_start_app_thread(loader);
}

// process messages

static bool loader_do_is_locked(Loader* loader) {
    return loader->app_data.thread != NULL;
}

static LoaderStatus loader_do_start_by_id(
    Loader* loader,
    const char* id,
    const char* args,
    FuriString* error_message
) {
    // check lock
    if (loader_do_is_locked(loader)) {
        const char* current_thread_name =
            furi_thread_get_name(furi_thread_get_id(loader->app_data.thread));
        return loader_make_status_error(
            LoaderStatusErrorAppStarted,
            error_message,
            "Loader is locked, please close the \"%s\" first",
            current_thread_name
        );
    }

    const AppManifest* manifest = app_manifest_registry_find_by_id(id);
    if (manifest == NULL) {
        return loader_make_status_error(
            LoaderStatusErrorUnknownApp,
            error_message,
            "Application \"%s\" not found",
            id
        );
    }

    loader_start_app(loader, manifest, args);
    return loader_make_success_status(error_message);
}

static bool loader_do_lock(Loader* loader) {
    if (loader->app_data.thread) {
        return false;
    }

    loader->app_data.thread = (FuriThread*)LOADER_MAGIC_THREAD_VALUE;
    return true;
}

static void loader_do_unlock(Loader* loader) {
    furi_check(loader->app_data.thread == (FuriThread*)LOADER_MAGIC_THREAD_VALUE);
    loader->app_data.thread = NULL;
}

static void loader_do_app_closed(Loader* loader) {
    furi_assert(loader->app_data.thread);

    furi_thread_join(loader->app_data.thread);
    FURI_LOG_I(
        TAG,
        "App %s returned: %li",
        loader->app_data.app->manifest->id,
        furi_thread_get_return_code(loader->app_data.thread)
    );

    if (loader->app_data.args) {
        free(loader->app_data.args);
        loader->app_data.args = NULL;
    }

    if (loader->app_data.app) {
        furi_app_free(loader->app_data.app);
        loader->app_data.app = NULL;
    } else {
        assert(loader->app_data.thread == NULL);
        furi_thread_free(loader->app_data.thread);
    }

    loader->app_data.thread = NULL;

    FURI_LOG_I(
        TAG,
        "Application stopped. Free heap: %zu",
        heap_caps_get_free_size(MALLOC_CAP_DEFAULT)
    );

    LoaderEvent event;
    event.type = LoaderEventTypeApplicationStopped;
    furi_pubsub_publish(loader->pubsub, &event);
}

// app

_Noreturn int32_t loader_main(void* p) {
    UNUSED(p);

    Loader* loader = loader_alloc();
    furi_record_create(RECORD_LOADER, loader);

    LoaderMessage message;
    while (true) {
        if (furi_message_queue_get(loader->queue, &message, FuriWaitForever) == FuriStatusOk) {
            switch (message.type) {
                case LoaderMessageTypeStartByName:
                    message.status_value->value = loader_do_start_by_id(
                        loader,
                        message.start.id,
                        message.start.args,
                        message.start.error_message
                    );
                    api_lock_unlock(message.api_lock);
                    break;
                case LoaderMessageTypeIsLocked:
                    message.bool_value->value = loader_do_is_locked(loader);
                    api_lock_unlock(message.api_lock);
                    break;
                case LoaderMessageTypeAppClosed:
                    loader_do_app_closed(loader);
                    break;
                case LoaderMessageTypeLock:
                    message.bool_value->value = loader_do_lock(loader);
                    api_lock_unlock(message.api_lock);
                    break;
                case LoaderMessageTypeUnlock:
                    loader_do_unlock(loader);
                    break;
            }
        }
    }
}

const AppManifest loader_app = {
    .id = "loader",
    .name = "Loader",
    .icon = NULL,
    .type = AppTypeService,
    .entry_point = &loader_main,
    .stack_size = AppStackSizeNormal
};
