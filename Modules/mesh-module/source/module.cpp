// SPDX-License-Identifier: Apache-2.0
#include <mesh/mesh.h>
#include <mesh/mesh_service.h>
#include <mesh/module.h>

#include <service/manager.h>

#include <tactility/error.h>
#include <tactility/module.h>

extern "C" {

static error_t start() {
    return service_manager_add(&mesh_service_manifest, mesh_service_should_auto_start());
}

static error_t stop() {
    const error_t error = service_manager_stop(mesh_service_manifest.id);
    if (error != ERROR_NONE && error != ERROR_NOT_FOUND) {
        return error;
    }
    return service_manager_remove(mesh_service_manifest.id);
}

static const ModuleSymbol mesh_module_symbols[] = {
    DEFINE_MODULE_SYMBOL(mesh_set_enabled),
    DEFINE_MODULE_SYMBOL(mesh_is_enabled),
    DEFINE_MODULE_SYMBOL(mesh_get_state),
    DEFINE_MODULE_SYMBOL(mesh_state_to_string),
    DEFINE_MODULE_SYMBOL(mesh_set_node_name),
    DEFINE_MODULE_SYMBOL(mesh_get_node_name),
    DEFINE_MODULE_SYMBOL(mesh_is_available),
    DEFINE_MODULE_SYMBOL(mesh_get_backend_count),
    DEFINE_MODULE_SYMBOL(mesh_get_backend_id),
    DEFINE_MODULE_SYMBOL(mesh_set_backend),
    DEFINE_MODULE_SYMBOL(mesh_get_backend),
    DEFINE_MODULE_SYMBOL(mesh_get_preset_count),
    DEFINE_MODULE_SYMBOL(mesh_get_preset_name),
    DEFINE_MODULE_SYMBOL(mesh_set_preset),
    DEFINE_MODULE_SYMBOL(mesh_get_preset),
    DEFINE_MODULE_SYMBOL(mesh_send_text),
    DEFINE_MODULE_SYMBOL(mesh_cancel),
    DEFINE_MODULE_SYMBOL(mesh_raw_send),
    DEFINE_MODULE_SYMBOL(mesh_message_get),
    DEFINE_MODULE_SYMBOL(mesh_history_get),
    DEFINE_MODULE_SYMBOL(mesh_get_nodes),
    DEFINE_MODULE_SYMBOL(mesh_get_channels),
    DEFINE_MODULE_SYMBOL(mesh_set_channel),
    DEFINE_MODULE_SYMBOL(mesh_event_subscribe),
    DEFINE_MODULE_SYMBOL(mesh_event_unsubscribe),
    DEFINE_MODULE_SYMBOL(mesh_event_poll),
    MODULE_SYMBOL_TERMINATOR
};

Module mesh_module = {
    .name = "mesh",
    .start = start,
    .stop = stop,
    .drivers = nullptr,
    .symbols = mesh_module_symbols,
    .internal = nullptr,
};

}
