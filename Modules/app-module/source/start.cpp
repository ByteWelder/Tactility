// SPDX-License-Identifier: Apache-2.0
#include <app/start.h>

#include <app/private/ledger.h>
#include <app/private/manager_internal.h>

#include <tactility/concurrent/mutex.h>

namespace {

// Looks @a id up in the manifest registry, then delegates to app_manager_start_internal(). The
// only path that requires a registered manifest; app_execute*() (app/execute.h) bypasses this
// entirely.
error_t start_internal_by_id(const char* id, AppInstanceId parent_instance_id, int argc, const char* const argv[], const AppStreamBinding* bindings, size_t binding_count, AppInstanceId* out_app_instance_id) {
    auto& ledger = app_ledger();

    mutex_lock(&ledger.mutex);
    auto manifest_iterator = ledger.manifests.find(id);
    if (manifest_iterator == ledger.manifests.end()) {
        mutex_unlock(&ledger.mutex);
        return ERROR_NOT_FOUND;
    }
    const AppManifest* manifest = manifest_iterator->second;
    mutex_unlock(&ledger.mutex);

    return app_manager_start_internal(manifest, manifest->location, manifest->stack, parent_instance_id, argc, argv, bindings, binding_count, out_app_instance_id);
}

} // namespace

extern "C" {

error_t app_start(const char* id, int argc, const char* const argv[], AppInstanceId* out_app_instance_id) {
    return start_internal_by_id(id, 0, argc, argv, nullptr, 0, out_app_instance_id);
}

error_t app_start_for_result(const char* id, int argc, const char* const argv[], AppInstanceId parent_instance_id, AppInstanceId* out_app_instance_id) {
    return start_internal_by_id(id, parent_instance_id, argc, argv, nullptr, 0, out_app_instance_id);
}

error_t app_start_with_streams(const char* id, const AppStreamBinding* bindings, size_t binding_count, AppInstanceId* out_app_instance_id) {
    return start_internal_by_id(id, 0, 0, nullptr, bindings, binding_count, out_app_instance_id);
}

error_t app_start_for_result_with_streams(const char* id, int argc, const char* const argv[], const AppStreamBinding* bindings, size_t binding_count, AppInstanceId parent_instance_id, AppInstanceId* out_app_instance_id) {
    return start_internal_by_id(id, parent_instance_id, argc, argv, bindings, binding_count, out_app_instance_id);
}

} // extern "C"
