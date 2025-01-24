#include "tt_app_manifest.h"

#include <Check.h>
#include <app/ElfApp.h>
#include <app/AppCompatC.h>

#define TAG "tt_app"

extern "C" {

void tt_app_register(
    const ExternalAppManifest* manifest
) {
#ifdef ESP_PLATFORM
    assert((manifest->createData == nullptr) == (manifest->destroyData == nullptr));
    tt::app::setElfAppManifest(
        manifest->name,
        manifest->icon,
        (tt::app::CreateData)manifest->createData,
        (tt::app::DestroyData)manifest->destroyData,
        (tt::app::OnStart)manifest->onStart,
        (tt::app::OnStop)manifest->onStop,
        (tt::app::OnShow)manifest->onShow,
        (tt::app::OnHide)manifest->onHide,
        (tt::app::OnResult)manifest->onResult
    );
#else
    tt_crash("TactilityC is intended for PC/Simulator");
#endif
}

}
