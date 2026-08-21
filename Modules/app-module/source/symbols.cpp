// SPDX-License-Identifier: Apache-2.0
#include <app/event.h>
#include <app/install.h>
#include <app/manager.h>
#include <app/metadata.h>
#include <app/paths.h>
#include <app/scheduler.h>

#include <service/manager.h>

#include <tactility/error.h>
#include <tactility/module.h>

extern "C" {

extern ServiceManifest app_internal_loader_service_manifest;

const ModuleSymbol app_module_symbols[] = {
    // app/event
    DEFINE_MODULE_SYMBOL(app_event_subscribe),
    DEFINE_MODULE_SYMBOL(app_event_unsubscribe),
    DEFINE_MODULE_SYMBOL(app_event_emit),
    DEFINE_MODULE_SYMBOL(app_event_await),
    // app/install
    DEFINE_MODULE_SYMBOL(app_get_install_path),
    DEFINE_MODULE_SYMBOL(app_install),
    DEFINE_MODULE_SYMBOL(app_uninstall),
    // app/manager
    DEFINE_MODULE_SYMBOL(app_manager_start),
    DEFINE_MODULE_SYMBOL(app_manager_start_with_parameters),
    DEFINE_MODULE_SYMBOL(app_manager_start_for_result),
    DEFINE_MODULE_SYMBOL(app_manager_stop),
    DEFINE_MODULE_SYMBOL(app_manager_get_state),
    DEFINE_MODULE_SYMBOL(app_manager_find_manifest),
    DEFINE_MODULE_SYMBOL(app_manager_for_each_manifest),
    DEFINE_MODULE_SYMBOL(app_manager_add),
    DEFINE_MODULE_SYMBOL(app_manager_remove),
    DEFINE_MODULE_SYMBOL(app_manager_get_topmost_instance_id),
    DEFINE_MODULE_SYMBOL(app_manager_get_topmost_app_id),
    DEFINE_MODULE_SYMBOL(app_manager_install_path_add),
    DEFINE_MODULE_SYMBOL(app_manager_install_path_scan),
    DEFINE_MODULE_SYMBOL(app_manager_install_path_uninstall),
    // app/metadata
    DEFINE_MODULE_SYMBOL(app_metadata_parse),
    // app/paths
    DEFINE_MODULE_SYMBOL(app_paths_get_user_data_directory),
    DEFINE_MODULE_SYMBOL(app_paths_get_user_data_path),
    DEFINE_MODULE_SYMBOL(app_paths_get_assets_directory),
    DEFINE_MODULE_SYMBOL(app_paths_get_assets_path),
    // app/scheduler
    DEFINE_MODULE_SYMBOL(app_scheduler_current_app_id),
    // terminator
    MODULE_SYMBOL_TERMINATOR
};

static error_t start() {
    return service_manager_add(&app_internal_loader_service_manifest, /*auto_start=*/true);
}

static error_t stop() {
    return service_manager_remove(app_internal_loader_service_manifest.id);
}

Module app_module = {
    .name = "app",
    .start = start,
    .stop = stop,
    .drivers = nullptr,
    .symbols = app_module_symbols,
    .internal = nullptr
};

}
