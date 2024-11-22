#include "app_i.h"

namespace tt {

#define TAG "app"

void AppInstance::setState(AppState newState) {
    mutex.acquire(TtWaitForever);
    state = newState;
    mutex.release();
}

AppState AppInstance::getState() {
    mutex.acquire(TtWaitForever);
    auto result = state;
    mutex.release();
    return result;
}

/** TODO: Make this thread-safe.
 * In practice, the bundle is writeable, so someone could be writing to it
 * while it is being accessed from another thread.
 * Consider creating MutableBundle vs Bundle.
 * Consider not exposing bundle, but expose `app_get_bundle_int(key)` methods with locking in it.
 */
const AppManifest& AppInstance::getManifest() {
    return manifest;
}

AppFlags AppInstance::getFlags() {
    mutex.acquire(TtWaitForever);
    auto result = flags;
    mutex.release();
    return result;
}

void AppInstance::setFlags(AppFlags newFlags) {
    mutex.acquire(TtWaitForever);
    flags = newFlags;
    mutex.release();
}

_Nullable void* AppInstance::getData() {
    mutex.acquire(TtWaitForever);
    auto result = data;
    mutex.release();
    return result;
}

void AppInstance::setData(void* newData) {
    mutex.acquire(TtWaitForever);
    data = newData;
    mutex.release();
}

const Bundle& AppInstance::getParameters() {
    return parameters;
}

App tt_app_alloc(const AppManifest& manifest, const Bundle& parameters) {
    auto* instance = new AppInstance(manifest, parameters);
    return static_cast<App>(instance);
}

void tt_app_free(App app) {
    auto* instance = static_cast<AppInstance*>(app);
    delete instance;
}

void tt_app_set_state(App app, AppState state) {
    static_cast<AppInstance*>(app)->setState(state);
}

AppState tt_app_get_state(App app) {
    return static_cast<AppInstance*>(app)->getState();
}

const AppManifest& tt_app_get_manifest(App app) {
    return static_cast<AppInstance*>(app)->getManifest();
}

AppFlags tt_app_get_flags(App app) {
    return static_cast<AppInstance*>(app)->getFlags();
}

void tt_app_set_flags(App app, AppFlags flags) {
    return static_cast<AppInstance*>(app)->setFlags(flags);
}

void* tt_app_get_data(App app) {
    return static_cast<AppInstance*>(app)->getData();
}

void tt_app_set_data(App app, void* data) {
    return static_cast<AppInstance*>(app)->setData(data);
}

const Bundle& tt_app_get_parameters(App app) {
    return static_cast<AppInstance*>(app)->getParameters();
}

} // namespace
