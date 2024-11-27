#include "app/AppInstance.h"

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
const Manifest& AppInstance::getManifest() const {
    return manifest;
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

_Nullable void* AppInstance::getData() const {
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

const Bundle& AppInstance::getParameters() const {
    return parameters;
}

void AppInstance::setResult(Result result, const Bundle& bundle) {
    std::unique_ptr<ResultHolder> new_holder(new ResultHolder(result, bundle));
    resultHolder = std::move(new_holder);
}

} // namespace
