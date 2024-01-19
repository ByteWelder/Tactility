#include "app_manifest.h"
#include "core_extra_defines.h"
#include "lvgl.h"
#include "thread.h"

static void app_show(App app, lv_obj_t* parent) {
    UNUSED(app);

    lv_obj_t* heap_info = lv_label_create(parent);
    lv_label_set_recolor(heap_info, true);
    lv_obj_set_width(heap_info, 200);
    lv_obj_set_style_text_align(heap_info, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text_fmt(
        heap_info,
        "Heap available:\n%d / %d",
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        heap_caps_get_total_size(MALLOC_CAP_INTERNAL)
    );
    lv_obj_align(heap_info, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t* spi_info = lv_label_create(parent);
    lv_label_set_recolor(spi_info, true);
    lv_obj_set_width(spi_info, 200);
    lv_obj_set_style_text_align(spi_info, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text_fmt(
        spi_info,
        "SPI available\n%d / %d",
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        heap_caps_get_total_size(MALLOC_CAP_SPIRAM)
    );
    lv_obj_align(spi_info, LV_ALIGN_CENTER, 0, 20);
}

AppManifest system_info_app = {
    .id = "systeminfo",
    .name = "System Info",
    .icon = NULL,
    .type = AppTypeSystem,
    .on_start = NULL,
    .on_stop = NULL,
    .on_show = &app_show
};
