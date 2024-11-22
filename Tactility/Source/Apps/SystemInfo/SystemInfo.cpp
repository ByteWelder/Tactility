#include "Assets.h"
#include "lvgl.h"
#include "Tactility.h"
#include "Ui/Toolbar.h"

namespace tt::app::system_info {

static size_t get_heap_free() {
#ifdef ESP_PLATFORM
    return heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
#else
    return 4096 * 1024;
#endif
}

static size_t get_heap_total() {
#ifdef ESP_PLATFORM
    return heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
#else
    return 8192 * 1024;
#endif
}

static size_t get_spi_free() {
#ifdef ESP_PLATFORM
    return heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
#else
    return 4096 * 1024;
#endif
}

static size_t get_spi_total() {
#ifdef ESP_PLATFORM
    return heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
#else
    return 8192 * 1024;
#endif
}

static void add_memory_bar(lv_obj_t* parent, const char* label, size_t used, size_t total) {
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);

    lv_obj_t* left_label = lv_label_create(container);
    lv_label_set_text(left_label, label);
    lv_obj_set_width(left_label, 60);

    lv_obj_t* bar = lv_bar_create(container);
    lv_obj_set_flex_grow(bar, 1);

    if (total > 0) {
        lv_bar_set_range(bar, 0, (int32_t)total);
    } else {
        lv_bar_set_range(bar, 0, 1);
    }

    lv_bar_set_value(bar, (int32_t)used, LV_ANIM_OFF);

    lv_obj_t* bottom_label = lv_label_create(parent);
    lv_label_set_text_fmt(bottom_label, "%u / %u kB", (used / 1024), (total / 1024));
    lv_obj_set_width(bottom_label, LV_PCT(100));
    lv_obj_set_style_text_align(bottom_label, LV_TEXT_ALIGN_RIGHT, 0);
}

static void on_show(App app, lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lvgl::toolbar_create_for_app(parent, app);

    // This wrapper automatically has its children added vertically underneath eachother
    lv_obj_t* wrapper = lv_obj_create(parent);
    lv_obj_set_style_border_width(wrapper, 0, 0);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);

    // Wrapper for the memory usage bars
    lv_obj_t* memory_label = lv_label_create(wrapper);
    lv_label_set_text(memory_label, "Memory usage");
    lv_obj_t* memory_wrapper = lv_obj_create(wrapper);
    lv_obj_set_flex_flow(memory_wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(memory_wrapper, LV_PCT(100), LV_SIZE_CONTENT);

    add_memory_bar(memory_wrapper, "Heap", get_heap_total() - get_heap_free(), get_heap_total());
    add_memory_bar(memory_wrapper, "SPI", get_spi_total() - get_spi_free(), get_spi_total());

#ifdef ESP_PLATFORM
    // Build info
    lv_obj_t* build_info_label = lv_label_create(wrapper);
    lv_label_set_text(build_info_label, "Build info");
    lv_obj_t* build_info_wrapper = lv_obj_create(wrapper);
    lv_obj_set_flex_flow(build_info_wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(build_info_wrapper, LV_PCT(100), LV_SIZE_CONTENT);

    lv_obj_t* esp_idf_version = lv_label_create(build_info_wrapper);
    lv_label_set_text_fmt(esp_idf_version, "IDF version: %d.%d.%d", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
#endif


}

extern const AppManifest manifest = {
    .id = "systeminfo",
    .name = "System Info",
    .icon = TT_ASSETS_APP_ICON_SYSTEM_INFO,
    .type = AppTypeSystem,
    .on_start = nullptr,
    .on_stop = nullptr,
    .on_show = &on_show
};

} // namespace

