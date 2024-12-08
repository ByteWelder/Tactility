#ifdef ESP_PLATFORM

#include "elf_symbol.h"

#include "app/SelectionDialog.h"

#ifdef __cplusplus
extern "C" {
#endif

const struct esp_elfsym elf_symbols[] {
    ESP_ELFSYM_EXPORT(tt_app_selectiondialog_start),
    ESP_ELFSYM_END
};

void tt_init_tactility_c() {
    elf_set_custom_symbols(elf_symbols);
}

#ifdef __cplusplus
}
#endif

#else // PC

void tt_init_tactility_c() {
}

#endif // ESP_PLATFORM