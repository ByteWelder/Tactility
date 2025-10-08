#include <private/elf_symbol.h>
#include <cstddef>

#include <symbols/esp_event.h>

#include <esp_event.h>

const esp_elfsym esp_event_symbols[] = {
    ESP_ELFSYM_EXPORT(esp_event_loop_create),
    ESP_ELFSYM_EXPORT(esp_event_loop_delete),
    ESP_ELFSYM_EXPORT(esp_event_loop_create_default),
    ESP_ELFSYM_EXPORT(esp_event_loop_delete_default),
    ESP_ELFSYM_EXPORT(esp_event_loop_run),
    ESP_ELFSYM_EXPORT(esp_event_handler_register),
    ESP_ELFSYM_EXPORT(esp_event_handler_register_with),
    ESP_ELFSYM_EXPORT(esp_event_handler_instance_register_with),
    ESP_ELFSYM_EXPORT(esp_event_handler_instance_register),
    ESP_ELFSYM_EXPORT(esp_event_handler_unregister),
    ESP_ELFSYM_EXPORT(esp_event_handler_unregister_with),
    ESP_ELFSYM_EXPORT(esp_event_handler_instance_unregister_with),
    ESP_ELFSYM_EXPORT(esp_event_handler_instance_unregister),
    ESP_ELFSYM_EXPORT(esp_event_post),
    ESP_ELFSYM_EXPORT(esp_event_post_to),
    ESP_ELFSYM_EXPORT(esp_event_isr_post),
    ESP_ELFSYM_EXPORT(esp_event_isr_post_to),
    // delimiter
    ESP_ELFSYM_END
};
