#ifdef ESP_PLATFORM

#include "elf_symbol.h"

#include "tt_app.h"
#include "tt_app_selectiondialog.h"
#include "tt_bundle.h"
#include "tt_lvgl_spinner.h"
#include "tt_lvgl_toolbar.h"
#include "tt_message_queue.h"
#include "tt_mutex.h"

#include <lvgl.h>

extern "C" {

const struct esp_elfsym elf_symbols[] {
    // Tactility
    ESP_ELFSYM_EXPORT(tt_app_selectiondialog_start),
    ESP_ELFSYM_EXPORT(tt_bundle_alloc),
    ESP_ELFSYM_EXPORT(tt_bundle_free),
    ESP_ELFSYM_EXPORT(tt_bundle_opt_bool),
    ESP_ELFSYM_EXPORT(tt_bundle_opt_int32),
    ESP_ELFSYM_EXPORT(tt_bundle_opt_string),
    ESP_ELFSYM_EXPORT(tt_bundle_put_bool),
    ESP_ELFSYM_EXPORT(tt_bundle_put_int32),
    ESP_ELFSYM_EXPORT(tt_bundle_put_string),
    ESP_ELFSYM_EXPORT(tt_set_app_manifest),
    ESP_ELFSYM_EXPORT(tt_lvgl_toolbar_create),
    ESP_ELFSYM_EXPORT(tt_lvgl_toolbar_create_simple),
    ESP_ELFSYM_EXPORT(tt_message_queue_alloc),
    ESP_ELFSYM_EXPORT(tt_message_queue_free),
    ESP_ELFSYM_EXPORT(tt_message_queue_put),
    ESP_ELFSYM_EXPORT(tt_message_queue_get),
    ESP_ELFSYM_EXPORT(tt_message_queue_get_capacity),
    ESP_ELFSYM_EXPORT(tt_message_queue_get_message_size),
    ESP_ELFSYM_EXPORT(tt_message_queue_get_count),
    ESP_ELFSYM_EXPORT(tt_message_queue_reset),
    ESP_ELFSYM_EXPORT(tt_mutex_alloc),
    ESP_ELFSYM_EXPORT(tt_mutex_free),
    ESP_ELFSYM_EXPORT(tt_mutex_lock),
    ESP_ELFSYM_EXPORT(tt_mutex_unlock),
    // tt::lvgl
    ESP_ELFSYM_EXPORT(tt_lvgl_spinner_create),
    // lv_obj
    ESP_ELFSYM_EXPORT(lv_obj_add_event_cb),
    ESP_ELFSYM_EXPORT(lv_obj_align),
    ESP_ELFSYM_EXPORT(lv_obj_align_to),
    ESP_ELFSYM_EXPORT(lv_obj_get_parent),
    ESP_ELFSYM_EXPORT(lv_obj_get_height),
    ESP_ELFSYM_EXPORT(lv_obj_get_width),
    ESP_ELFSYM_EXPORT(lv_obj_get_coords),
    ESP_ELFSYM_EXPORT(lv_obj_get_x),
    ESP_ELFSYM_EXPORT(lv_obj_get_display),
    ESP_ELFSYM_EXPORT(lv_obj_get_y),
    ESP_ELFSYM_EXPORT(lv_obj_get_content_width),
    ESP_ELFSYM_EXPORT(lv_obj_get_content_height),
    ESP_ELFSYM_EXPORT(lv_obj_center),
    ESP_ELFSYM_EXPORT(lv_color_make),
    ESP_ELFSYM_EXPORT(lv_obj_remove_event_cb),
    ESP_ELFSYM_EXPORT(lv_obj_get_user_data),
    ESP_ELFSYM_EXPORT(lv_obj_set_user_data),
    ESP_ELFSYM_EXPORT(lv_obj_set_pos),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_margin_hor),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_margin_ver),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_margin_top),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_margin_bottom),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_margin_left),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_margin_right),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_margin_all),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_pad_all),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_pad_hor),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_pad_ver),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_pad_top),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_pad_bottom),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_pad_left),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_pad_right),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_border_width),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_border_opa),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_border_post),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_border_side),
    ESP_ELFSYM_EXPORT(lv_obj_set_style_border_color),
    ESP_ELFSYM_EXPORT(lv_obj_set_x),
    ESP_ELFSYM_EXPORT(lv_obj_set_y),
    ESP_ELFSYM_EXPORT(lv_obj_set_width),
    ESP_ELFSYM_EXPORT(lv_obj_set_height),
    ESP_ELFSYM_EXPORT(lv_theme_get_color_primary),
    ESP_ELFSYM_EXPORT(lv_theme_get_color_secondary),
    // lv_button
    ESP_ELFSYM_EXPORT(lv_button_create),
    // lv_label
    ESP_ELFSYM_EXPORT(lv_label_create),
    ESP_ELFSYM_EXPORT(lv_label_cut_text),
    ESP_ELFSYM_EXPORT(lv_label_get_long_mode),
    ESP_ELFSYM_EXPORT(lv_label_set_long_mode),
    ESP_ELFSYM_EXPORT(lv_label_get_text),
    ESP_ELFSYM_EXPORT(lv_label_set_text),
    ESP_ELFSYM_EXPORT(lv_label_set_text_fmt),
    // lv_switch
    ESP_ELFSYM_EXPORT(lv_switch_create),
    // lv_checkbox
    ESP_ELFSYM_EXPORT(lv_checkbox_create),
    ESP_ELFSYM_EXPORT(lv_checkbox_set_text),
    ESP_ELFSYM_EXPORT(lv_checkbox_get_text),
    ESP_ELFSYM_EXPORT(lv_checkbox_set_text_static),
    // lv_bar
    ESP_ELFSYM_EXPORT(lv_bar_create),
    ESP_ELFSYM_EXPORT(lv_bar_get_max_value),
    ESP_ELFSYM_EXPORT(lv_bar_get_min_value),
    ESP_ELFSYM_EXPORT(lv_bar_get_mode),
    ESP_ELFSYM_EXPORT(lv_bar_get_start_value),
    ESP_ELFSYM_EXPORT(lv_bar_get_value),
    ESP_ELFSYM_EXPORT(lv_bar_set_mode),
    ESP_ELFSYM_EXPORT(lv_bar_set_range),
    ESP_ELFSYM_EXPORT(lv_bar_set_start_value),
    ESP_ELFSYM_EXPORT(lv_bar_set_value),
    ESP_ELFSYM_EXPORT(lv_bar_is_symmetrical),
    // lv_dropdown
    ESP_ELFSYM_EXPORT(lv_dropdown_create),
    ESP_ELFSYM_EXPORT(lv_dropdown_add_option),
    ESP_ELFSYM_EXPORT(lv_dropdown_clear_options),
    ESP_ELFSYM_EXPORT(lv_dropdown_close),
    ESP_ELFSYM_EXPORT(lv_dropdown_get_dir),
    ESP_ELFSYM_EXPORT(lv_dropdown_get_list),
    ESP_ELFSYM_EXPORT(lv_dropdown_get_option_count),
    ESP_ELFSYM_EXPORT(lv_dropdown_get_option_index),
    ESP_ELFSYM_EXPORT(lv_dropdown_get_options),
    ESP_ELFSYM_EXPORT(lv_dropdown_set_dir),
    ESP_ELFSYM_EXPORT(lv_dropdown_set_options),
    ESP_ELFSYM_EXPORT(lv_dropdown_set_options_static),
    ESP_ELFSYM_EXPORT(lv_dropdown_set_selected),
    ESP_ELFSYM_EXPORT(lv_dropdown_set_selected_highlight),
    ESP_ELFSYM_EXPORT(lv_dropdown_set_symbol),
    ESP_ELFSYM_EXPORT(lv_dropdown_set_text),
    ESP_ELFSYM_EXPORT(lv_dropdown_open),
    // lv_list
    ESP_ELFSYM_EXPORT(lv_list_create),
    ESP_ELFSYM_EXPORT(lv_list_add_text),
    ESP_ELFSYM_EXPORT(lv_list_add_button),
    ESP_ELFSYM_EXPORT(lv_list_get_button_text),
    ESP_ELFSYM_EXPORT(lv_list_set_button_text),
    // lv_textarea
    ESP_ELFSYM_EXPORT(lv_textarea_create),
    ESP_ELFSYM_EXPORT(lv_textarea_get_accepted_chars),
    ESP_ELFSYM_EXPORT(lv_textarea_get_label),
    ESP_ELFSYM_EXPORT(lv_textarea_get_max_length),
    ESP_ELFSYM_EXPORT(lv_textarea_get_one_line),
    ESP_ELFSYM_EXPORT(lv_textarea_set_one_line),
    ESP_ELFSYM_EXPORT(lv_textarea_set_accepted_chars),
    ESP_ELFSYM_EXPORT(lv_textarea_set_align),
    ESP_ELFSYM_EXPORT(lv_textarea_set_password_bullet),
    ESP_ELFSYM_EXPORT(lv_textarea_set_password_mode),
    ESP_ELFSYM_EXPORT(lv_textarea_set_password_show_time),
    ESP_ELFSYM_EXPORT(lv_textarea_set_placeholder_text),
    ESP_ELFSYM_EXPORT(lv_textarea_set_text),
    ESP_ELFSYM_EXPORT(lv_textarea_set_text_selection),
    // delimiter
    ESP_ELFSYM_END
};

void tt_init_tactility_c() {
    elf_set_custom_symbols(elf_symbols);
}

}

#else // Simulator

extern "C" {

void tt_init_tactility_c() {
}

}

#endif // ESP_PLATFORM
