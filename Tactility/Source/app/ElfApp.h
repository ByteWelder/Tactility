#pragma once

#include "AppManifest.h"

#ifdef ESP_PLATFORM

namespace tt::app {

typedef void* (*CreateData)();
typedef void (*DestroyData)(void* data);
typedef void (*OnStart)(void* appContext, void* _Nullable data);
typedef void (*OnStop)(void* appContext, void* _Nullable data);
typedef void (*OnShow)(void* appContext, void* _Nullable data, lv_obj_t* parent);
typedef void (*OnHide)(void* appContext, void* _Nullable data);
typedef void (*OnResult)(void* appContext, void* _Nullable data, Result result, Bundle* resultData);

void setElfAppManifest(
    const char* name,
    const char* _Nullable icon,
    CreateData _Nullable createData,
    DestroyData _Nullable destroyData,
    OnStart _Nullable onStart,
    OnStop _Nullable onStop,
    OnShow _Nullable onShow,
    OnHide _Nullable onHide,
    OnResult _Nullable onResult
);

/**
 * @return the app ID based on the executable's file path.
 */
std::string getElfAppId(const std::string& filePath);

/**
 * @return true when registration was done, false when app was already registered
 */
bool registerElfApp(const std::string& filePath);

std::shared_ptr<App> createElfApp(const std::shared_ptr<AppManifest>& manifest);

}
#endif // ESP_PLATFORM
