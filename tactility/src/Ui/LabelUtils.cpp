#include "LabelUtils.h"
#include "TactilityCore.h"

namespace tt::lvgl {

#define TAG "tt_lv_label"

static long file_get_size(FILE* file) {
    long original_offset = ftell(file);

    if (fseek(file, 0, SEEK_END) != 0) {
        TT_LOG_E(TAG, "fseek failed");
        return -1;
    }

    long file_size = ftell(file);
    if (file_size == -1) {
        TT_LOG_E(TAG, "Could not get file length");
        return -1;
    }

    if (fseek(file, original_offset, SEEK_SET) != 0) {
        TT_LOG_E(TAG, "fseek Failed");
        return -1;
    }

    return file_size;
}

static char* str_alloc_from_file(const char* filepath) {
    FILE* file = fopen(filepath, "rb");

    if (file == nullptr) {
        TT_LOG_E(TAG, "Failed to open %s", filepath);
        return nullptr;
    }

    long content_length = file_get_size(file);

    auto* text_buffer = static_cast<char*>(malloc(content_length + 1));
    if (text_buffer == nullptr) {
        TT_LOG_E(TAG, "Insufficient memory. Failed to allocate %ldl bytes.", content_length);
        return nullptr;
    }

    int buffer;
    uint32_t buffer_offset = 0;
    text_buffer[0] = 0;
    while ((buffer = fgetc(file)) != EOF && buffer_offset < content_length) {
        text_buffer[buffer_offset] = (char)buffer;
        buffer_offset++;
    }
    text_buffer[buffer_offset] = 0;

    fclose(file);
    return text_buffer;
}

void label_set_text_file(lv_obj_t* label, const char* filepath) {
    char* text = str_alloc_from_file(filepath);
    lv_label_set_text(label, text);
    free(text);
}

} // namespace
