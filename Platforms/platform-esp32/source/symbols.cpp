// SPDX-License-Identifier: Apache-2.0
#include <tactility/module.h>

#include <esp_event.h>

extern "C" {

const ModuleSymbol platform_esp32_symbols[] = {
    DEFINE_MODULE_SYMBOL(esp_event_loop_create),
    DEFINE_MODULE_SYMBOL(esp_event_loop_delete),
    DEFINE_MODULE_SYMBOL(esp_event_loop_create_default),
    DEFINE_MODULE_SYMBOL(esp_event_loop_delete_default),
    DEFINE_MODULE_SYMBOL(esp_event_loop_run),
    DEFINE_MODULE_SYMBOL(esp_event_handler_register),
    DEFINE_MODULE_SYMBOL(esp_event_handler_register_with),
    DEFINE_MODULE_SYMBOL(esp_event_handler_instance_register_with),
    DEFINE_MODULE_SYMBOL(esp_event_handler_instance_register),
    DEFINE_MODULE_SYMBOL(esp_event_handler_unregister),
    DEFINE_MODULE_SYMBOL(esp_event_handler_unregister_with),
    DEFINE_MODULE_SYMBOL(esp_event_handler_instance_unregister_with),
    DEFINE_MODULE_SYMBOL(esp_event_handler_instance_unregister),
    DEFINE_MODULE_SYMBOL(esp_event_post),
    DEFINE_MODULE_SYMBOL(esp_event_post_to),
    DEFINE_MODULE_SYMBOL(esp_event_isr_post),
    DEFINE_MODULE_SYMBOL(esp_event_isr_post_to),
    MODULE_SYMBOL_TERMINATOR
};

}
