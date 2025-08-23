#include "Tactility/file/FileLock.h"

#include <Tactility/hal/Device.h>
#include <Tactility/hal/sdcard/SdCardDevice.h>
#include <Tactility/Mutex.h>

namespace tt::file {

class NoLock : public Lock {
    bool lock(TickType_t timeout) const override { return true; }
    bool unlock() const override { return true; }
};

static std::shared_ptr<Lock> noLock = std::make_shared<NoLock>();

std::shared_ptr<Lock> getLock(const std::string& path) {
    auto sdcard_lock = hal::sdcard::findSdCardLock(path);
    if (sdcard_lock != nullptr) {
        return sdcard_lock;
    } else {
        return noLock;
    }
}

}
