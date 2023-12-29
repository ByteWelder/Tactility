#include "loader.h"
#include <sys/cdefs.h>

#include "app_i.h"
#include "app_manifest_registry.h"
#include "loader_i.h"

//#include <dialogs/dialogs.h>
#include "app_manifest.h"

#define TAG "Loader"
#define LOADER_MAGIC_THREAD_VALUE 0xDEADBEEF

// API

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

/*
LoaderStatus loader_start_with_gui_error(Loader* loader, const char* name, const char* args) {
    FuriString* error_message = furi_string_alloc();
    LoaderStatus status = loader_start(loader, name, args, error_message);

    if (status == LoaderStatusErrorUnknownApp &&
        loader_find_external_application_by_name(name) != NULL) {
        // Special case for external apps
        DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS);
        DialogMessage* message = dialog_message_alloc();
        dialog_message_set_header(message, "Update needed", 64, 3, AlignCenter, AlignTop);
        dialog_message_set_buttons(message, NULL, NULL, NULL);
        dialog_message_set_icon(message, &I_DolphinCommon_56x48, 72, 17);
        dialog_message_set_text(
            message, "Update firmware\nto run this app", 3, 26, AlignLeft, AlignTop
        );
        dialog_message_show(dialogs, message);
        dialog_message_free(message);
        furi_record_close(RECORD_DIALOGS);
    } else if (status == LoaderStatusErrorUnknownApp || status == LoaderStatusErrorInternal) {
        // TODO FL-3522: we have many places where we can emit a double start, ex: desktop, menu
        // so i prefer to not show LoaderStatusErrorAppStarted error message for now
        DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS);
        DialogMessage* message = dialog_message_alloc();
        dialog_message_set_header(message, "Error", 64, 0, AlignCenter, AlignTop);
        dialog_message_set_buttons(message, NULL, NULL, NULL);

        furi_string_replace(error_message, "/ext/apps/", "");
        furi_string_replace(error_message, ", ", "\n");
        furi_string_replace(error_message, ": ", "\n");

        dialog_message_set_text(
            message, furi_string_get_cstr(error_message), 64, 35, AlignCenter, AlignCenter
        );

        dialog_message_show(dialogs, message);
        dialog_message_free(message);
        furi_record_close(RECORD_DIALOGS);
    }

    furi_string_free(error_message);
    return status;
}
*/

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

void loader_show_menu(Loader* loader) {
    LoaderMessage message;
    message.type = LoaderMessageTypeShowMenu;
    furi_message_queue_put(loader->queue, &message, FuriWaitForever);
}

FuriPubSub* loader_get_pubsub(Loader* loader) {
    furi_assert(loader);
    // it's safe to return pubsub without locking
    // because it's never freed and loader is never exited
    // also the loader instance cannot be obtained until the pubsub is created
    return loader->pubsub;
}

// callbacks

static void loader_menu_closed_callback(void* context) {
    Loader* loader = context;
    LoaderMessage message;
    message.type = LoaderMessageTypeMenuClosed;
    furi_message_queue_put(loader->queue, &message, FuriWaitForever);
}

static void loader_applications_closed_callback(void* context) {
    Loader* loader = context;
    LoaderMessage message;
    message.type = LoaderMessageTypeApplicationsClosed;
    furi_message_queue_put(loader->queue, &message, FuriWaitForever);
}

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
//    loader->loader_menu = NULL;
//    loader->loader_applications = NULL;
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

    /* This flag is set by the debugger - to break on app start */
//        if (furi_hal_debug_is_gdb_session_active()) {
//            FURI_LOG_W(TAG, "Triggering BP for debugger");
//            /* After hitting this, you can set breakpoints in your .fap's code
//             * Note that you have to toggle breakpoints that were set before */
//            __asm volatile("bkpt 0");
//        }

    loader_start_app_thread(loader);
}

// process messages

static void loader_do_menu_show(Loader* loader) {
//    if (!loader->loader_menu) {
//        loader->loader_menu = loader_menu_alloc(loader_menu_closed_callback, loader);
//    }
}

static void loader_do_menu_closed(Loader* loader) {
//    if (loader->loader_menu) {
//        loader_menu_free(loader->loader_menu);
//        loader->loader_menu = NULL;
//    }
}

static void loader_do_applications_show(Loader* loader) {
//    if (!loader->loader_applications) {
//        loader->loader_applications =
//            loader_applications_alloc(loader_applications_closed_callback, loader);
//    }
}

static void loader_do_applications_closed(Loader* loader) {
//    if (loader->loader_applications) {
//        loader_applications_free(loader->loader_applications);
//        loader->loader_applications = NULL;
//    }
}

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

    // Don't start self
    if (strcmp(id, LOADER_APPLICATIONS_NAME) == 0) {
        loader_do_applications_show(loader);
        return loader_make_success_status(error_message);
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
    FURI_LOG_I(TAG, "App returned: %li", furi_thread_get_return_code(loader->app_data.thread));

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

    // TODO: implement this part from memmgr for esp!
//    FURI_LOG_I(TAG, "Application stopped. Free heap: %zu", memmgr_get_free_heap());

    LoaderEvent event;
    event.type = LoaderEventTypeApplicationStopped;
    furi_pubsub_publish(loader->pubsub, &event);
}

// app

_Noreturn int32_t loader_main(void* p) {
    UNUSED(p);

    Loader* loader = loader_alloc();
    furi_record_create(RECORD_LOADER, loader);

//    FURI_LOG_I(TAG, "Executing system start hooks");
//    for (size_t i = 0; i < NANOBAKE_ON_SYSTEM_START_COUNT; i++) {
//        NANOBAKE_ON_SYSTEM_START[i]();
//    }
//
//    if ((furi_hal_rtc_get_boot_mode() == FuriHalRtcBootModeNormal) && FLIPPER_AUTORUN_APP_NAME &&
//        strlen(FLIPPER_AUTORUN_APP_NAME)) {
//        FURI_LOG_I(TAG, "Starting autorun app: %s", FLIPPER_AUTORUN_APP_NAME);
//        loader_do_start_by_id(loader, FLIPPER_AUTORUN_APP_ID, NULL, NULL);
//    }

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
                case LoaderMessageTypeShowMenu:
                    loader_do_menu_show(loader);
                    break;
                case LoaderMessageTypeMenuClosed:
                    loader_do_menu_closed(loader);
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
                case LoaderMessageTypeApplicationsClosed:
                    loader_do_applications_closed(loader);
                    break;
            }
        }
    }
}

const AppManifest loader_app = {
    .id = LOADER_APPLICATIONS_NAME,
    .name = "Loader",
    .icon = NULL,
    .type = AppTypeService,
    .entry_point = &loader_main,
    .stack_size = AppStackSizeNormal
};
