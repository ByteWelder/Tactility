#pragma once

#include "AppCompatC.h"
#include "AppManifest.h"

#ifdef ESP_PLATFORM

namespace tt::app {

bool startElfApp(const std::string& filePath);

void setElfAppManifest(
    const char* name,
    const char* _Nullable icon,
    OnStart _Nullable onStart,
    OnStop _Nullable onStop,
    OnShow _Nullable onShow,
    OnHide _Nullable onHide,
    OnResult _Nullable onResult
);

}
#endif // ESP_PLATFORM
