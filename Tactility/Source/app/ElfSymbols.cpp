#include "ElfSymbols.h"
#include "elf_symbol.h"

#include "app/selectiondialog/SelectionDialog.h"

extern "C" {

const struct esp_elfsym elf_symbols[] {
    ESP_ELFSYM_EXPORT(tt_app_selectiondialog_start),
    ESP_ELFSYM_END
};

}

void initElfSymbols() {
    elf_set_custom_symbols(elf_symbols);
}
