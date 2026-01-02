#include <private/elf_symbol.h>
#include <cstddef>

#include <symbols/freertos.h>

#include <Tactility/freertoscompat/RTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

const esp_elfsym freertos_symbols[] = {
    ESP_ELFSYM_EXPORT(vTaskDelete),
    ESP_ELFSYM_EXPORT(xTaskCreate),
    ESP_ELFSYM_EXPORT(xTaskCreatePinnedToCore),
    ESP_ELFSYM_EXPORT(xEventGroupCreate),
    ESP_ELFSYM_EXPORT(xEventGroupClearBits),
    ESP_ELFSYM_EXPORT(xEventGroupSetBits),
    ESP_ELFSYM_EXPORT(xEventGroupWaitBits),
    ESP_ELFSYM_EXPORT(vEventGroupDelete),
    // delimiter
    ESP_ELFSYM_END
};
