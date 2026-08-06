// SPDX-License-Identifier: Apache-2.0
#include <tactility/module.h>
#include <service/instance.h>
#include <service/manager.h>
#include <service/paths.h>

const ModuleSymbol service_module_symbols[] = {
    // service/service_instance
    DEFINE_MODULE_SYMBOL(service_instance_construct),
    DEFINE_MODULE_SYMBOL(service_instance_destruct),
    DEFINE_MODULE_SYMBOL(service_instance_get_manifest),
    DEFINE_MODULE_SYMBOL(service_instance_get_data),
    DEFINE_MODULE_SYMBOL(service_instance_get_state),
    // service/service_manager
    DEFINE_MODULE_SYMBOL(service_manager_add),
    DEFINE_MODULE_SYMBOL(service_manager_remove),
    DEFINE_MODULE_SYMBOL(service_manager_find_manifest),
    DEFINE_MODULE_SYMBOL(service_manager_start),
    DEFINE_MODULE_SYMBOL(service_manager_stop),
    DEFINE_MODULE_SYMBOL(service_manager_get_state),
    DEFINE_MODULE_SYMBOL(service_manager_find_instance),
    // service/service_paths
    DEFINE_MODULE_SYMBOL(service_paths_get_user_data_directory),
    DEFINE_MODULE_SYMBOL(service_paths_get_user_data_path),
    DEFINE_MODULE_SYMBOL(service_paths_get_assets_directory),
    DEFINE_MODULE_SYMBOL(service_paths_get_assets_path),
    // terminator
    MODULE_SYMBOL_TERMINATOR
};

Module service_module = {
    .name = "service",
    .drivers = nullptr,
    .symbols = service_module_symbols,
    .internal = nullptr
};
