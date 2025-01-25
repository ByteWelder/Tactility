#include "app/AppInstance.h"
#include "app/AppInstancePaths.h"

namespace tt::app {

#define TAG "app"

void AppInstance::setState(State newState) {
    mutex.acquire(TtWaitForever);
    state = newState;
    mutex.release();
}

State AppInstance::getState() const {
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
const AppManifest& AppInstance::getManifest() const {
    assert(manifest != nullptr);
    return *manifest;
}

Flags AppInstance::getFlags() const {
    mutex.acquire(TtWaitForever);
    auto result = flags;
    mutex.release();
    return result;
}

void AppInstance::setFlags(Flags newFlags) {
    mutex.acquire(TtWaitForever);
    flags = newFlags;
    mutex.release();
}

std::shared_ptr<const Bundle> AppInstance::getParameters() const {
    mutex.acquire(TtWaitForever);
    std::shared_ptr<const Bundle> result = parameters;
    mutex.release();
    return result;
}

std::unique_ptr<Paths> AppInstance::getPaths() const {
    assert(manifest != nullptr);
    return std::make_unique<AppInstancePaths>(*manifest);
}

} // namespace
