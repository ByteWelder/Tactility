#include "app_manifest_registry.h"

#include "m-dict.h"
#include "m_cstr_dup.h"
#include "mutex.h"
#include "tactility_core.h"

#define TAG "app_registry"

DICT_DEF2(AppManifestDict, const char*, M_CSTR_DUP_OPLIST, const AppManifest*, M_PTR_OPLIST)

#define APP_REGISTRY_FOR_EACH(manifest_var_name, code_to_execute)                                               \
    {                                                                                                           \
        app_registry_lock();                                                                                    \
        AppManifestDict_it_t it;                                                                                \
        for (AppManifestDict_it(it, app_manifest_dict); !AppManifestDict_end_p(it); AppManifestDict_next(it)) { \
            const AppManifest*(manifest_var_name) = AppManifestDict_cref(it)->value;                            \
            code_to_execute;                                                                                    \
        }                                                                                                       \
        app_registry_unlock();                                                                                  \
    }

AppManifestDict_t app_manifest_dict;
Mutex hash_mutex = NULL;

void tt_app_manifest_registry_init() {
    tt_assert(hash_mutex == NULL);
    hash_mutex = tt_mutex_alloc(MutexTypeNormal);
    AppManifestDict_init(app_manifest_dict);
}

void app_registry_lock() {
    tt_assert(hash_mutex != NULL);
    tt_mutex_acquire(hash_mutex, TtWaitForever);
}

void app_registry_unlock() {
    tt_assert(hash_mutex != NULL);
    tt_mutex_release(hash_mutex);
}

void tt_app_manifest_registry_add(const AppManifest* manifest) {
    TT_LOG_I(TAG, "adding %s", manifest->id);

    app_registry_lock();
    AppManifestDict_set_at(app_manifest_dict, manifest->id, manifest);
    app_registry_unlock();
}

void tt_app_manifest_registry_remove(const AppManifest* manifest) {
    TT_LOG_I(TAG, "removing %s", manifest->id);
    app_registry_lock();
    AppManifestDict_erase(app_manifest_dict, manifest->id);
    app_registry_unlock();
}

const AppManifest _Nullable* tt_app_manifest_registry_find_by_id(const char* id) {
    app_registry_lock();
    const AppManifest _Nullable** manifest = AppManifestDict_get(app_manifest_dict, id);
    app_registry_unlock();
    return (manifest != NULL) ? *manifest : NULL;
}

void tt_app_manifest_registry_for_each_of_type(AppType type, void* _Nullable context, AppManifestCallback callback) {
    APP_REGISTRY_FOR_EACH(manifest, {
        if (manifest->type == type) {
            callback(manifest, context);
        }
    });
}

void tt_app_manifest_registry_for_each(AppManifestCallback callback, void* _Nullable context) {
    APP_REGISTRY_FOR_EACH(manifest, {
        callback(manifest, context);
    });
}
