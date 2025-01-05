#include "LabelUtils.h"
#include "file/File.h"

namespace tt::lvgl {

#define TAG "tt_lv_label"

bool label_set_text_file(lv_obj_t* label, const char* filepath) {
    auto text = file::readString(filepath);
    if (text != nullptr) {
        lv_label_set_text(label, (const char*)text.get());
        return true;
    } else {
        return false;
    }
}

} // namespace
