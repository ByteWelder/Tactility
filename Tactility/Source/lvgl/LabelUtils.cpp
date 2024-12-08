#include "LabelUtils.h"
#include "file/File.h"

namespace tt::lvgl {

#define TAG "tt_lv_label"

void label_set_text_file(lv_obj_t* label, const char* filepath) {
    auto text = file::readString(filepath);
    lv_label_set_text(label, (const char*)text.get());
}

} // namespace
