#include "Tactility/hal/SdCard.h"
#include "Tactility/hal/Device.h"

namespace tt::hal {

std::shared_ptr<SdCard> _Nullable findSdCard(const std::string& path) {
    auto sdcards = findDevices<SdCard>(Device::Type::SdCard);
    for (auto& sdcard : sdcards) {
        if (sdcard->isMounted() && path.starts_with(sdcard->getMountPath())) {
            return sdcard;
        }
    }

    return nullptr;
}

}
