#include "tt_app_manifest.h"

#include <Check.h>
#include <app/ElfApp.h>
#include <app/AppCompatC.h>

#define TAG "tt_app"

extern "C" {

void tt_set_app_manifest(
    const char* name,
    const char* _Nullable icon,
    AppCreateData _Nullable createData,
    AppDestroyData _Nullable destroyData,
    AppOnStart _Nullable onStart,
    AppOnStop _Nullable onStop,
    AppOnShow _Nullable onShow,
    AppOnHide _Nullable onHide,
    AppOnResult _Nullable onResult
) {
#ifdef ESP_PLATFORM
    tt_assert((createData == nullptr) == (destroyData == nullptr));
    tt::app::setElfAppManifest(
        name,
        icon,
        (tt::app::CreateData)createData,
        (tt::app::DestroyData)destroyData,
        (tt::app::OnStart)onStart,
        (tt::app::OnStop)onStop,
        (tt::app::OnShow)onShow,
        (tt::app::OnHide)onHide,
        (tt::app::OnResult)onResult
    );
#else
    tt_crash("TactilityC is intended for PC/Simulator");
#endif
}

}
