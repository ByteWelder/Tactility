#include "Tactility/lvgl/LabelUtils.h"
#include "Tactility/file/File.h"
#include "Tactility/file/FileLock.h"

namespace tt::lvgl {

constexpr auto* TAG = "LabelUtils";

bool label_set_text_file(lv_obj_t* label, const char* filepath) {
    std::unique_ptr<uint8_t[]> text;
    file::getLock(filepath)->withLock([&text, filepath] {
        text = file::readString(filepath);
    });

    if (text != nullptr) {
        lv_label_set_text(label, reinterpret_cast<const char*>(text.get()));
        return true;
    } else {
        return false;
    }
}

} // namespace
