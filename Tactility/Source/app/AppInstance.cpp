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
const AppManifest& AppInstance::getManifest() const {
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

std::shared_ptr<void> _Nullable AppInstance::getData() const {
    mutex.acquire(TtWaitForever);
    auto result = data;
    mutex.release();
    return result;
}

void AppInstance::setData(std::shared_ptr<void> newData) {
    mutex.acquire(TtWaitForever);
    data = newData;
    mutex.release();
}

std::shared_ptr<const Bundle> AppInstance::getParameters() const {
    mutex.acquire(TtWaitForever);
    std::shared_ptr<const Bundle> result = parameters;
    mutex.release();
    return result;
}

void AppInstance::setResult(Result result) {
    std::unique_ptr<ResultHolder> new_holder(new ResultHolder(result));
    mutex.acquire(TtWaitForever);
    resultHolder = std::move(new_holder);
    mutex.release();
}

void AppInstance::setResult(Result result, std::shared_ptr<const Bundle> bundle) {
    std::unique_ptr<ResultHolder> new_holder(new ResultHolder(result, std::move(bundle)));
    mutex.acquire(TtWaitForever);
    resultHolder = std::move(new_holder);
    mutex.release();
}

bool AppInstance::hasResult() const {
    mutex.acquire(TtWaitForever);
    bool has_result = resultHolder != nullptr;
    mutex.release();
    return has_result;
}

} // namespace
