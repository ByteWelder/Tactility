#include "tt_app_manifest.h"

#include <Tactility/Check.h>
#include <Tactility/app/ElfApp.h>

#define TAG "tt_app"

extern "C" {

void tt_app_register(
    const ExternalAppManifest* manifest
) {
#ifdef ESP_PLATFORM
    assert((manifest->createData == nullptr) == (manifest->destroyData == nullptr));
    setElfAppManifest(
        manifest->name,
        manifest->icon,
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
