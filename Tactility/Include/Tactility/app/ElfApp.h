#pragma once

#include "AppManifest.h"

#ifdef ESP_PLATFORM

namespace tt::app {

typedef void* (*CreateData)();
typedef void (*DestroyData)(void* data);
typedef void (*OnCreate)(void* appContext, void* _Nullable data);
typedef void (*OnDestroy)(void* appContext, void* _Nullable data);
typedef void (*OnShow)(void* appContext, void* _Nullable data, lv_obj_t* parent);
typedef void (*OnHide)(void* appContext, void* _Nullable data);
typedef void (*OnResult)(void* appContext, void* _Nullable data, LaunchId launchId, Result result, Bundle* resultData);

void setElfAppManifest(
    const char* name,
    const char* _Nullable icon,
    CreateData _Nullable createData,
    DestroyData _Nullable destroyData,
    OnCreate _Nullable onCreate,
    OnDestroy _Nullable onDestroy,
    OnShow _Nullable onShow,
    OnHide _Nullable onHide,
    OnResult _Nullable onResult
);

/**
 * @return the app ID based on the executable's file path.
 */
std::string getElfAppId(const std::string& filePath);

void registerElfApp(const std::string& filePath);

std::shared_ptr<App> createElfApp(const std::shared_ptr<AppManifest>& manifest);

}
#endif // ESP_PLATFORM
