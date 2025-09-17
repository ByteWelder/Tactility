#include "tt_app_manifest.h"

#include <Tactility/Check.h>
#include <Tactility/app/ElfApp.h>

extern "C" {

constexpr auto TAG = "tt_app";

void tt_app_register(
    const ExternalAppManifest* manifest
) {
#ifdef ESP_PLATFORM
    assert((manifest->createData == nullptr) == (manifest->destroyData == nullptr));
    tt::app::setElfAppParameters(
        manifest->createData,
        manifest->destroyData,
        manifest->onCreate,
        manifest->onDestroy,
        manifest->onShow,
        manifest->onHide,
        reinterpret_cast<tt::app::OnResult>(manifest->onResult)
    );
#else
    tt_crash("TactilityC is not intended for PC/Simulator");
#endif
}

}
