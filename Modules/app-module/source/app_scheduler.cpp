// SPDX-License-Identifier: Apache-2.0
#include <app/private/app_ledger.h>
#include <app/private/app_scheduler.h>

#include <app/event.h>
#include <app/loader.h>

#include <service/instance.h>
#include <service/manager.h>

#include <tactility/concurrent/thread.h>
#include <tactility/log.h>

#include <new>

#define TAG "app_scheduler"

namespace {

struct ThreadContext {
    const AppLoaderApi* loader;
    void* runtime;
    uint32_t app_instance_id;
    int argc;
    char** argv;
};

void set_state(uint32_t app_instance_id, AppInstanceState state) {
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    auto iterator = ledger.instances.find(app_instance_id);
    if (iterator != ledger.instances.end()) {
        iterator->second.state = state;
    }
    mutex_unlock(&ledger.mutex);
}

Thread* get_thread(uint32_t app_instance_id) {
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    auto iterator = ledger.instances.find(app_instance_id);
    Thread* thread = (iterator != ledger.instances.end()) ? iterator->second.thread : nullptr;
    mutex_unlock(&ledger.mutex);
    return thread;
}

void set_thread(uint32_t app_instance_id, Thread* thread) {
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    auto iterator = ledger.instances.find(app_instance_id);
    if (iterator != ledger.instances.end()) {
        iterator->second.thread = thread;
    }
    mutex_unlock(&ledger.mutex);
}

const char* loader_service_id_for(AppLocationType type) {
    return (type == APP_LOCATION_MEMORY) ? APP_LOADER_MEMORY_SERVICE_ID : APP_LOADER_PATH_SERVICE_ID;
}

const AppLoaderApi* find_loader_api(AppLocationType type) {
    ServiceInstance* instance = service_manager_find_instance(loader_service_id_for(type));
    if (instance == nullptr) {
        return nullptr;
    }
    return static_cast<const AppLoaderApi*>(service_instance_get_data(instance));
}

// If this instance was launched via app_manager_start_for_result(), delivers @a result (its
// own AppMainFn/AppLoaderApi::run() return value) to its parent. No-op for a top-level instance
// (parent_id == 0).
void deliver_result_to_parent_if_any(uint32_t app_instance_id, int32_t result) {
    auto& ledger = app_ledger();

    uint32_t parent_id;
    AppEvent event { .type = APP_EVENT_RESULT, .timestamp = 0, .result = {} };

    mutex_lock(&ledger.mutex);
    auto iterator = ledger.instances.find(app_instance_id);
    if (iterator == ledger.instances.end()) {
        mutex_unlock(&ledger.mutex);
        return;
    }
    parent_id = iterator->second.parent_id;
    event.result.launch_id = app_instance_id;
    event.result.result = result;
    mutex_unlock(&ledger.mutex);

    if (parent_id != 0) {
        app_event_emit(parent_id, &event);
    }
}

int32_t thread_main(void* context) {
    auto* ctx = static_cast<ThreadContext*>(context);

    set_state(ctx->app_instance_id, APP_INSTANCE_STATE_ACTIVE);

    int32_t result = ctx->loader->run(ctx->runtime, ctx->app_instance_id, ctx->argc, ctx->argv);

    ctx->loader->unload(ctx->runtime);

    deliver_result_to_parent_if_any(ctx->app_instance_id, result);

    // A safe default terminal marker for CLOSE (and any other exit): an app that calls
    // app_manager_finish() already marked itself Stopped before returning, so this is a no-op
    // for it - but it's still needed as the terminal marker for any other exit path.
    set_state(ctx->app_instance_id, APP_INSTANCE_STATE_STOPPED);

    app_ledger_free_arguments(ctx->argc, ctx->argv);
    delete ctx;
    return result;
}

} // namespace

extern "C" {

error_t app_scheduler_start(uint32_t app_instance_id, AppLocation location, int argc, char* argv[]) {
    const AppLoaderApi* loader = find_loader_api(location.type);
    if (loader == nullptr) {
        LOG_E(TAG, "No app loader is registered (service '%s' not found)", loader_service_id_for(location.type));
        app_ledger_free_arguments(argc, argv);
        return ERROR_NOT_FOUND;
    }

    void* runtime = nullptr;
    error_t load_result = loader->load(location, &runtime);
    if (load_result != ERROR_NONE) {
        app_ledger_free_arguments(argc, argv);
        return load_result;
    }

    auto* context = new (std::nothrow) ThreadContext { loader, runtime, app_instance_id, argc, argv };
    if (context == nullptr) {
        loader->unload(runtime);
        app_ledger_free_arguments(argc, argv);
        return ERROR_OUT_OF_MEMORY;
    }

    // -1 (no affinity) matches the FreeRTOS POSIX/simulator port; ESP-IDF's tskNO_AFFINITY is
    // a numerically equivalent SMP-only constant not available in the plain FreeRTOS-Kernel port.
    Thread* thread = thread_alloc_full("app", 8192, thread_main, context, -1);
    if (thread == nullptr) {
        delete context;
        loader->unload(runtime);
        app_ledger_free_arguments(argc, argv);
        return ERROR_OUT_OF_MEMORY;
    }

    set_thread(app_instance_id, thread);

    error_t start_result = thread_start(thread);
    if (start_result != ERROR_NONE) {
        set_thread(app_instance_id, nullptr);
        thread_free(thread);
        delete context;
        loader->unload(runtime);
        app_ledger_free_arguments(argc, argv);
        return start_result;
    }

    return ERROR_NONE;
}

error_t app_scheduler_stop(uint32_t app_instance_id, TickType_t join_timeout) {
    Thread* thread = get_thread(app_instance_id);
    if (thread != nullptr) {
        AppEvent event { .type = APP_EVENT_CLOSE, .timestamp = 0, .result = {} };
        app_event_emit(app_instance_id, &event);

        if (thread_join(thread, join_timeout, pdMS_TO_TICKS(10)) != ERROR_NONE) {
            LOG_W(TAG, "App instance %u did not stop in time", app_instance_id);
            return ERROR_TIMEOUT;
        }
        thread_free(thread);
        set_thread(app_instance_id, nullptr);
    }

    set_state(app_instance_id, APP_INSTANCE_STATE_STOPPED);

    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    ledger.instances.erase(app_instance_id);
    mutex_unlock(&ledger.mutex);

    return ERROR_NONE;
}

} // extern "C"
