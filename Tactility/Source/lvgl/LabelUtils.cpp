#include <Tactility/lvgl/LabelUtils.h>
#include <Tactility/file/File.h>
#include <Tactility/hal/sdcard/SdCardDevice.h>

namespace tt::lvgl {

#define TAG "tt_lv_label"

bool label_set_text_file(lv_obj_t* label, const char* filepath) {
    auto text = hal::sdcard::withSdCardLock<std::unique_ptr<uint8_t[]>>(std::string(filepath), [filepath]() {
        return file::readString(filepath);
    });

    if (text != nullptr) {
        lv_label_set_text(label, reinterpret_cast<const char*>(text.get()));
        return true;
    } else {
        return false;
    }
}

} // namespace
