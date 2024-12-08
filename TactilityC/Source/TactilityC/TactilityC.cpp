#ifdef ESP_PLATFORM

#include "TactilityC.h"
#include "elf_symbol.h"

#include "app/SelectionDialog.h"

#ifdef __cplusplus
extern "C" {
#endif

const struct esp_elfsym elf_symbols[] {
    ESP_ELFSYM_EXPORT(tt_app_selectiondialog_start),
    ESP_ELFSYM_END
};

void initElfSymbols() {
    elf_set_custom_symbols(elf_symbols);
}

#ifdef __cplusplus
}
#endif

#endif // ESP_PLATFORM