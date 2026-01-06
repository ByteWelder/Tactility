#include <Tactility/hal/sdcard/SdCardMounting.h>
#include <Tactility/hal/sdcard/SdCardDevice.h>

#include <Tactility/Logger.h>

#include <format>

namespace tt::hal::sdcard {

static const auto LOGGER = Logger("SdCardMounting");
constexpr auto* TT_SDCARD_MOUNT_POINT = "/sdcard";

static void mount(const std::shared_ptr<SdCardDevice>& sdcard, const std::string& path) {
   LOGGER.info("Mounting sdcard at {}", path);
   if (!sdcard->mount(path)) {
       LOGGER.warn("SD card mount failed for {} (init can continue)", path);
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
