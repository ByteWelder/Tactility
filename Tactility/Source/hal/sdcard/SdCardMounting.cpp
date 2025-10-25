#include <Tactility/hal/sdcard/SdCardMounting.h>
#include <Tactility/hal/sdcard/SdCardDevice.h>

#include <format>

namespace tt::hal::sdcard {

constexpr auto* TAG = "SdCardMounting";
constexpr auto* TT_SDCARD_MOUNT_POINT = "/sdcard";

static void mount(const std::shared_ptr<SdCardDevice>& sdcard, const std::string& path) {
   TT_LOG_I(TAG, "Mounting sdcard at %s", path.c_str());
   if (!sdcard->mount(path)) {
       TT_LOG_W(TAG, "SD card mount failed for %s (init can continue)", path.c_str());
   }
}

static std::string getMountPath(int index, int count) {
    return (count == 1) ? TT_SDCARD_MOUNT_POINT : std::format("{}{}", TT_SDCARD_MOUNT_POINT, index);
}

void mountAll() {
    const auto sdcards = hal::findDevices<SdCardDevice>(Device::Type::SdCard);
    // Numbered mount path name
    for (int i = 0; i < sdcards.size(); i++) {
        auto sdcard = sdcards[i];
        if (!sdcard->isMounted() && sdcard->getMountBehaviour() == SdCardDevice::MountBehaviour::AtBoot) {
            std::string mount_path = getMountPath(i, sdcards.size());
            mount(sdcard, mount_path);
        }
    }
}

}
