#ifdef ESP_PLATFORM

#include "elf_symbol.h"

#include "app/App.h"
#include "app/SelectionDialog.h"
#include "lvgl/Toolbar.h"

#include "lvgl.h"

extern "C" {

const struct esp_elfsym elf_symbols[] {
    // Tactility
    ESP_ELFSYM_EXPORT(tt_app_selectiondialog_start),
    ESP_ELFSYM_EXPORT(tt_set_app_manifest),
    ESP_ELFSYM_EXPORT(tt_lvgl_toolbar_create),
    ESP_ELFSYM_EXPORT(tt_lvgl_toolbar_create_simple),
    // lv_obj
    ESP_ELFSYM_EXPORT(lv_obj_add_event_cb),
    ESP_ELFSYM_EXPORT(lv_obj_align),
    ESP_ELFSYM_EXPORT(lv_obj_align_to),
    ESP_ELFSYM_EXPORT(lv_obj_get_user_data),
    ESP_ELFSYM_EXPORT(lv_obj_set_user_data),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_margin_all),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_pad_all),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_border_width),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_border_color),
    // lv_button
    ESP_ELFSYM_EXPORT(lv_button_create),
    // lv_label
    ESP_ELFSYM_EXPORT(lv_label_create),
    ESP_ELFSYM_EXPORT(lv_label_set_text),
    ESP_ELFSYM_EXPORT(lv_label_set_text_fmt),
    ESP_ELFSYM_END
};

void tt_init_tactility_c() {
    elf_set_custom_symbols(elf_symbols);
}

#else // PC

void tt_init_tactility_c() {
}

#endif // ESP_PLATFORM

}

