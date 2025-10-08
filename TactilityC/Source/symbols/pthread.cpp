#include <private/elf_symbol.h>
#include <cstddef>

#include <symbols/pthread.h>

#include <pthread.h>

const esp_elfsym pthread_symbols[] = {
    ESP_ELFSYM_EXPORT(pthread_create),
    ESP_ELFSYM_EXPORT(pthread_attr_init),
    ESP_ELFSYM_EXPORT(pthread_attr_setstacksize),
    ESP_ELFSYM_EXPORT(pthread_detach),
    ESP_ELFSYM_EXPORT(pthread_join),
    ESP_ELFSYM_EXPORT(pthread_exit),
    // delimiter
    ESP_ELFSYM_END
};
