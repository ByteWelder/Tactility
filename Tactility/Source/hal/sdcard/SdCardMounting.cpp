#include <Tactility/hal/sdcard/SdCardMounting.h>
#include <Tactility/hal/sdcard/SdCardDevice.h>

namespace tt::hal::sdcard {

constexpr auto* TAG = "SdCardMounting";
constexpr auto* TT_SDCARD_MOUNT_POINT = "/sdcard";

static void mount(const std::shared_ptr<SdCardDevice>& sdcard, const std::string& path) {
    sdcard->getLock()->withLock([&sdcard, &path] {
       TT_LOG_I(TAG, "Mounting sdcard at %s", path.c_str());
       if (!sdcard->mount(path)) {
           TT_LOG_W(TAG, "SD card mount failed for %s (init can continue)", path.c_str());
       }
   });
}

void mountAll() {
    auto sdcards = hal::findDevices<SdCardDevice>(Device::Type::SdCard);
    if (!sdcards.empty()) {
        if (sdcards.size() == 1) {
            // Fixed mount path name
            auto sdcard = sdcards[0];
            if (!sdcard->isMounted()) {
                mount(sdcard, TT_SDCARD_MOUNT_POINT);
            }
        } else {
            // Numbered mount path name
            for (int i = 0; i < sdcards.size(); i++) {
                auto sdcard = sdcards[i];
                if (!sdcard->isMounted()) {
                    std::string mount_path = TT_SDCARD_MOUNT_POINT + std::to_string(i);
                    mount(sdcard, mount_path);
                }
            }
        }
    }
}

}
