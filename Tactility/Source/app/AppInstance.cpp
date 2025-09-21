#include <Tactility/app/AppInstance.h>
#include <Tactility/app/AppPaths.h>

namespace tt::app {

#define TAG "app"

void AppInstance::setState(State newState) {
    mutex.lock();
    state = newState;
    mutex.unlock();
}

State AppInstance::getState() const {
    mutex.lock();
    auto result = state;
    mutex.unlock();
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
    mutex.lock();
    auto result = flags;
    mutex.unlock();
    return result;
}

void AppInstance::setFlags(Flags newFlags) {
    mutex.lock();
    flags = newFlags;
    mutex.unlock();
}

std::shared_ptr<const Bundle> AppInstance::getParameters() const {
    mutex.lock();
    std::shared_ptr<const Bundle> result = parameters;
    mutex.unlock();
    return result;
}

std::unique_ptr<AppPaths> AppInstance::getPaths() const {
    assert(manifest != nullptr);
    return std::make_unique<AppPaths>(*manifest);
}

} // namespace
