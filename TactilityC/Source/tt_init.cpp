#ifdef ESP_PLATFORM

#include "private/elf_symbol.h"
#include "tt_app_alertdialog.h"
#include "tt_app_fileselection.h"
#include "tt_app_selectiondialog.h"

#include <cassert>
#include <cstring>
#include <ctype.h>
#include <driver/ledc.h>
#include <esp_heap_caps.h>
#include <miniz.h>
#include <sys/errno.h>
#include <vector>

#include <Tactility/Tactility.h>

#include <driver/i2s_common.h>
#include <driver/i2s_std.h>
#include <driver/gpio.h>

#ifdef CONFIG_IDF_TARGET_ESP32P4
#include <driver/ppa.h>
#include <esp_cache.h>
#endif

extern "C" {

const esp_elfsym main_symbols[] {
#ifdef __HAVE_LOCALE_INFO__
    // ctype.h
    ESP_ELFSYM_EXPORT(__locale_ctype_ptr),
#else
    ESP_ELFSYM_EXPORT(_ctype_),
#endif

    // Tactility
    ESP_ELFSYM_EXPORT(tt_app_fileselection_start_for_existing_file),
    ESP_ELFSYM_EXPORT(tt_app_fileselection_start_for_existing_or_new_file),
    ESP_ELFSYM_EXPORT(tt_app_fileselection_get_result_path),
    ESP_ELFSYM_EXPORT(tt_app_selectiondialog_start),
    ESP_ELFSYM_EXPORT(tt_app_alertdialog_start),
    // delimiter
    ESP_ELFSYM_END,
};

uintptr_t resolve_symbol(const esp_elfsym* source, const char* symbolName) {
    const esp_elfsym* symbol_iterator = source;
    while (symbol_iterator->name != nullptr) {
        if (strcmp(symbol_iterator->name, symbolName) == 0) {
            return reinterpret_cast<uintptr_t>(symbol_iterator->sym);
        }
        symbol_iterator++;
    }
    return 0;
}

uintptr_t tt_symbol_resolver(const char* symbolName) {
    static const std::vector all_symbols = {
        main_symbols,
    };

    for (const auto* symbols : all_symbols) {
        const uintptr_t address = resolve_symbol(symbols, symbolName);
        if (address != 0) {
            return address;
        }
    }

    uintptr_t symbol_address;
    if (module_resolve_symbol_global(symbolName, &symbol_address)) {
        return symbol_address;
    }

    return 0;
}

void tt_init_tactility_c() {
    elf_set_symbol_resolver(tt_symbol_resolver);
}

}

// extern "C"

#else // Simulator

extern "C" {

void tt_init_tactility_c() {
}

}

#endif // ESP_PLATFORM
