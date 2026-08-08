// SPDX-License-Identifier: Apache-2.0
#include <app/private/app_ledger.h>
#include <app/private/app_scheduler.h>
#include <app/event.h>
#include <app/instance.h>
#include <app/loader.h>
#include <app/scheduler.h>

#include <service/instance.h>
#include <service/manager.h>

#include <tactility/error.h>
#include <tactility/log.h>

#include <cstdint>
#include <cstdio>
#include <new>

constexpr auto* TAG = "app_scheduler";

// Slot 0 is reserved by ESP-IDF's pthread API (see TactilityKernel's Thread wrapper for the
// same convention/comment) - app tasks use slot 1 to stash their own app_instance_id, so any
// code running on an app's own task can retrieve it via app_scheduler_current_app_id() without
// needing it threaded through as a parameter.
constexpr size_t APP_INSTANCE_ID_THREAD_SLOT_INDEX = 1;

// Matches TactilityKernel's Thread wrapper's THREAD_PRIORITY_NORMAL.
constexpr UBaseType_t APP_TASK_PRIORITY = 4;

namespace {

struct TaskContext {
    const AppLoaderApi* loader;
    void* runtime;
    AppInstanceId app_instance_id;
    int argc;
    char** argv;
};

void set_state(AppInstanceId app_instance_id, AppInstanceState state) {
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    auto iterator = ledger.instances.find(app_instance_id);
    if (iterator != ledger.instances.end()) {
        iterator->second.state = state;
    }
    mutex_unlock(&ledger.mutex);
}

void set_task(AppInstanceId app_instance_id, TaskHandle_t task) {
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    auto iterator = ledger.instances.find(app_instance_id);
    if (iterator != ledger.instances.end()) {
        iterator->second.task = task;
    }
    mutex_unlock(&ledger.mutex);
}

// Registers the calling task to be notified when app_instance_id's task actually finishes
// running (see app_task_main()'s exit path), and reports whether there's anything to wait for.
// @return true if the instance's ledger entry still exists (a wait was registered); false if
// the task has already fully finished (and already given any notification it would have) -
// there is nothing left to wait for.
bool register_stop_waiter(AppInstanceId app_instance_id) {
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    auto iterator = ledger.instances.find(app_instance_id);
    bool exists = iterator != ledger.instances.end();
    if (exists) {
        iterator->second.stop_waiter = xTaskGetCurrentTaskHandle();
    }
    mutex_unlock(&ledger.mutex);
    return exists;
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
void deliver_result_to_parent_if_any(AppInstanceId app_instance_id, int32_t result) {
    auto& ledger = app_ledger();

    AppInstanceId parent_id;
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

void app_task_main(void* context) {
    auto* ctx = static_cast<TaskContext*>(context);

    check(pvTaskGetThreadLocalStoragePointer(nullptr, APP_INSTANCE_ID_THREAD_SLOT_INDEX) == nullptr);
    vTaskSetThreadLocalStoragePointer(nullptr, APP_INSTANCE_ID_THREAD_SLOT_INDEX, reinterpret_cast<void*>(static_cast<uintptr_t>(ctx->app_instance_id)));

    LOG_I(TAG, "Thread for %d started", ctx->app_instance_id);

    set_state(ctx->app_instance_id, APP_INSTANCE_STATE_ACTIVE);

    int32_t result = ctx->loader->run(ctx->runtime, ctx->app_instance_id, ctx->argc, ctx->argv);

    vTaskSetThreadLocalStoragePointer(nullptr, APP_INSTANCE_ID_THREAD_SLOT_INDEX, nullptr);

    ctx->loader->unload(ctx->runtime);

    deliver_result_to_parent_if_any(ctx->app_instance_id, result);

    // A safe default terminal marker for CLOSE (and any other exit): an app that calls
    // app_manager_finish() already marked itself Stopped before returning, so this is a no-op
    // for it - but it's still needed as the terminal marker for any other exit path.
    set_state(ctx->app_instance_id, APP_INSTANCE_STATE_STOPPED);

    app_ledger_free_arguments(ctx->argc, ctx->argv);

    AppInstanceId app_instance_id = ctx->app_instance_id;
    delete ctx;

    LOG_I(TAG, "Thread for %d finished", app_instance_id);

    // Erase the ledger entry before self-deleting, capturing whoever's blocked in
    // app_scheduler_stop() for this instance (if anyone) so they can be notified afterward.
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    auto iterator = ledger.instances.find(app_instance_id);
    TaskHandle_t stop_waiter = (iterator != ledger.instances.end()) ? iterator->second.stop_waiter : nullptr;
    ledger.instances.erase(app_instance_id);
    mutex_unlock(&ledger.mutex);

    // Signal completion as the literal last action before this task ceases to exist, so
    // app_scheduler_stop() can't observe "stopped" one step early (see its own comment) -
    // unlike watching the ledger entry disappear, this can only happen once the task is truly
    // done running.
    if (stop_waiter != nullptr) {
        xTaskNotifyGive(stop_waiter);
    }

    vTaskDelete(nullptr);
}

} // namespace

extern "C" {

error_t app_scheduler_start(AppInstanceId app_instance_id, AppLocation location, int argc, char* argv[]) {
    const AppLoaderApi* loader = find_loader_api(location.type);
    if (loader == nullptr) {
        LOG_E(TAG, "No app loader is registered (service '%s' not found)", loader_service_id_for(location.type));
        app_ledger_free_arguments(argc, argv);
        return ERROR_NOT_FOUND;
    }

    void* runtime = nullptr;
    error_t load_result = loader->load(location, &runtime);
    if (load_result != ERROR_NONE) {
        LOG_E(TAG, "Failed to load app: %s", error_to_string(load_result));
        app_ledger_free_arguments(argc, argv);
        return load_result;
    }

    auto* context = new (std::nothrow) TaskContext { loader, runtime, app_instance_id, argc, argv };
    if (context == nullptr) {
        LOG_E(TAG, "Failed to allocate app");
        loader->unload(runtime);
        app_ledger_free_arguments(argc, argv);
        return ERROR_OUT_OF_MEMORY;
    }

    char task_name[16];
    snprintf(task_name, sizeof(task_name), "app_%lu", static_cast<unsigned long>(app_instance_id));

    TaskHandle_t task_handle = nullptr;
    // 8192 bytes -> stack depth in words, matching what TactilityKernel's Thread wrapper does with the stack size it's given.
    // Created at idle priority so it can't preempt us before vTaskSuspend() below runs, then suspended immediately -
    // the ledger must record the handle (set_task()) before the task can possibly observe or erase its own entry.
    // (see app_scheduler_stop()'s liveness check and app_task_main()'s exit path)
    BaseType_t create_result = xTaskCreate(app_task_main, task_name, 8192 / sizeof(StackType_t), context, tskIDLE_PRIORITY, &task_handle);
    if (create_result != pdPASS) {
        delete context;
        loader->unload(runtime);
        app_ledger_free_arguments(argc, argv);
        return ERROR_OUT_OF_MEMORY;
    }
    vTaskSuspend(task_handle);

    set_task(app_instance_id, task_handle);
    vTaskPrioritySet(task_handle, APP_TASK_PRIORITY);
    vTaskResume(task_handle);

    return ERROR_NONE;
}

error_t app_scheduler_stop(AppInstanceId app_instance_id, TickType_t join_timeout) {
    // Drain any stale notification credit before registering as the waiter - otherwise a
    // leftover give from an unrelated earlier wait on this same task (e.g. a previous
    // app_scheduler_stop() call that timed out and only got notified afterward) could make the
    // take below return immediately for the wrong event. Mirrors app_event_await()'s same
    // defensive drain.
    ulTaskNotifyTake(pdTRUE, 0);

    if (register_stop_waiter(app_instance_id)) {
        AppEvent event { .type = APP_EVENT_CLOSE, .timestamp = 0, .result = {} };
        app_event_emit(app_instance_id, &event);

        // Blocks until app_task_main() gives this notification as the literal last thing it
        // does before vTaskDelete() - unlike polling the ledger for the task handle to clear,
        // this can't observe "stopped" while the task is still mid-exit (still running its own
        // cleanup/vTaskDelete()).
        if (ulTaskNotifyTake(pdTRUE, join_timeout) == 0) {
            LOG_W(TAG, "App instance %u did not stop in time", app_instance_id);
            return ERROR_TIMEOUT;
        }
    }

    set_state(app_instance_id, APP_INSTANCE_STATE_STOPPED);

    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    ledger.instances.erase(app_instance_id);
    mutex_unlock(&ledger.mutex);

    return ERROR_NONE;
}

AppInstanceId app_scheduler_current_app_id(void) {
    void* value = pvTaskGetThreadLocalStoragePointer(nullptr, APP_INSTANCE_ID_THREAD_SLOT_INDEX);
    return reinterpret_cast<uintptr_t>(value);
}

} // extern "C"
