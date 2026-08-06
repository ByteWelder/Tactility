// SPDX-License-Identifier: Apache-2.0
#include <app/manager.h>

#include <app/private/app_ledger.h>
#include <app/private/app_scheduler.h>

#include <tactility/log.h>

#include <cstring>

#define TAG "app_manager"

extern "C" {

error_t app_manager_add(const AppManifest* manifest) {
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    if (ledger.manifests.contains(manifest->id)) {
        mutex_unlock(&ledger.mutex);
        LOG_E(TAG, "Manifest with id '%s' is already registered", manifest->id);
        return ERROR_INVALID_ARGUMENT;
    }
    ledger.manifests[manifest->id] = manifest;
    mutex_unlock(&ledger.mutex);

    return ERROR_NONE;
}

error_t app_manager_remove(const char* id) {
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    auto iterator = ledger.manifests.find(id);
    if (iterator == ledger.manifests.end()) {
        mutex_unlock(&ledger.mutex);
        return ERROR_NOT_FOUND;
    }
    ledger.manifests.erase(iterator);
    mutex_unlock(&ledger.mutex);

    return ERROR_NONE;
}

const AppManifest* app_manager_find_manifest(const char* id) {
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    auto iterator = ledger.manifests.find(id);
    const AppManifest* manifest = (iterator != ledger.manifests.end()) ? iterator->second : nullptr;
    mutex_unlock(&ledger.mutex);
    return manifest;
}

void app_manager_for_each_manifest(AppManifestVisitorFn visitor, void* context) {
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    for (auto& [id, manifest] : ledger.manifests) {
        visitor(manifest, context);
    }
    mutex_unlock(&ledger.mutex);
}

namespace {

// Deep-copies argv (argc <= 0 => NULL, matching "no parameters"). Caller passes the result to
// app_scheduler_start(), which takes ownership regardless of outcome.
char** copy_arguments(int argc, const char* const argv[]) {
    if (argc <= 0) {
        return nullptr;
    }
    auto* copy = new char*[argc + 1];
    for (int i = 0; i < argc; i++) {
        size_t length = strlen(argv[i]);
        copy[i] = new char[length + 1];
        memcpy(copy[i], argv[i], length + 1);
    }
    copy[argc] = nullptr;
    return copy;
}

// Takes ownership of argv (already a deep copy, or NULL/argc==0) regardless of outcome -
// app_scheduler_start() frees it on any failure path, and the spawned task frees it once its
// run() returns.
error_t start_internal(const char* id, AppInstanceId parent_instance_id, int argc, char* argv[], AppInstanceId* out_app_instance_id) {
    const AppManifest* manifest = app_manager_find_manifest(id);
    if (manifest == nullptr) {
        app_ledger_free_arguments(argc, argv);
        return ERROR_NOT_FOUND;
    }

    auto& ledger = app_ledger();

    mutex_lock(&ledger.mutex);
    AppInstanceId target_id = ledger.next_instance_id++;
    AppInstanceRecord record { target_id, manifest, APP_INSTANCE_STATE_STARTING, nullptr };
    record.parent_id = parent_instance_id;
    ledger.instances[target_id] = record;
    mutex_unlock(&ledger.mutex);

    error_t result = app_scheduler_start(target_id, manifest->location, argc, argv);
    if (result != ERROR_NONE) {
        mutex_lock(&ledger.mutex);
        ledger.instances.erase(target_id);
        mutex_unlock(&ledger.mutex);
        return result;
    }

    *out_app_instance_id = target_id;
    return ERROR_NONE;
}

} // namespace

error_t app_manager_start(const char* id, AppInstanceId* out_app_instance_id) {
    return start_internal(id, 0, 0, nullptr, out_app_instance_id);
}

error_t app_manager_start_with_parameters(const char* id, int argc, const char* const argv[], AppInstanceId* out_app_instance_id) {
    return start_internal(id, 0, argc, copy_arguments(argc, argv), out_app_instance_id);
}

error_t app_manager_start_for_result(const char* id, AppInstanceId parent_instance_id, int argc, const char* const argv[], AppInstanceId* out_app_instance_id) {
    return start_internal(id, parent_instance_id, argc, copy_arguments(argc, argv), out_app_instance_id);
}

error_t app_manager_stop(AppInstanceId app_instance_id) {
    return app_scheduler_stop(app_instance_id, pdMS_TO_TICKS(2000));
}

error_t app_manager_finish(AppInstanceId app_instance_id) {
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    auto iterator = ledger.instances.find(app_instance_id);
    if (iterator != ledger.instances.end()) {
        iterator->second.state = APP_INSTANCE_STATE_STOPPED;
    }
    mutex_unlock(&ledger.mutex);
    return ERROR_NONE;
}

AppInstanceState app_manager_get_state(AppInstanceId app_instance_id) {
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    auto iterator = ledger.instances.find(app_instance_id);
    AppInstanceState state = (iterator != ledger.instances.end()) ? iterator->second.state : APP_INSTANCE_STATE_STOPPED;
    mutex_unlock(&ledger.mutex);
    return state;
}

error_t app_manager_get_topmost_instance_id(AppInstanceId* out_app_instance_id) {
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    AppInstanceId topmost_id = 0;
    for (auto& [instance_id, record] : ledger.instances) {
        // Instance ids are handed out in increasing order (AppLedger::next_instance_id), so
        // the highest Active id is also the most recently started one.
        if (record.state == APP_INSTANCE_STATE_ACTIVE && instance_id > topmost_id) {
            topmost_id = instance_id;
        }
    }
    mutex_unlock(&ledger.mutex);

    if (topmost_id == 0) {
        return ERROR_NOT_FOUND;
    }
    *out_app_instance_id = topmost_id;
    return ERROR_NONE;
}

error_t app_manager_get_topmost_app_id(char* buffer, size_t buffer_size) {
    if (buffer_size == 0) {
        return ERROR_BUFFER_OVERFLOW;
    }
    buffer[0] = '\0';

    AppInstanceId topmost_id = 0;
    error_t result = app_manager_get_topmost_instance_id(&topmost_id);
    if (result != ERROR_NONE) {
        return result;
    }

    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    auto iterator = ledger.instances.find(topmost_id);
    const char* app_id = (iterator != ledger.instances.end()) ? iterator->second.manifest->id : nullptr;
    mutex_unlock(&ledger.mutex);

    if (app_id == nullptr) {
        return ERROR_NOT_FOUND;
    }

    size_t length = strlen(app_id);
    if (length >= buffer_size) {
        buffer[0] = '\0';
        return ERROR_BUFFER_OVERFLOW;
    }
    memcpy(buffer, app_id, length + 1);
    return ERROR_NONE;
}

} // extern "C"
