#pragma once

#include "../Device.h"

#include <Tactility/TactilityCore.h>

namespace tt::hal::sdcard {

class SdCardDevice : public Device {

public:

    enum class State {
        Mounted,
        Unmounted,
        Error,
        Unknown
    };

    enum class MountBehaviour {
        AtBoot, /** Only mount at boot */
        Anytime /** Mount/dismount any time */
    };

private:

    MountBehaviour mountBehaviour;

public:

    explicit SdCardDevice(MountBehaviour mountBehaviour) : mountBehaviour(mountBehaviour) {}
    ~SdCardDevice() override = default;

    Type getType() const final { return Type::SdCard; };

    virtual bool mount(const std::string& mountPath) = 0;
    virtual bool unmount() = 0;
    virtual State getState() const = 0;
    /** Return empty string when not mounted or the mount path if mounted */
    virtual std::string getMountPath() const = 0;

    /** Non-null lock */
    virtual std::shared_ptr<Lock> getLock() const = 0;

    virtual MountBehaviour getMountBehaviour() const { return mountBehaviour; }
    bool isMounted() const { return getState() == State::Mounted; }
};

/** Return the SdCard device if the path is within the SdCard mounted path (path std::string::starts_with() check)*/
std::shared_ptr<SdCardDevice> _Nullable find(const std::string& path);

/**
 * Attempt to find an SD card that the specified belongs to,
 * and returns its lock if the SD card is mounted. Otherwise it returns nullptr.
 * @param[in] a path on a file system (e.g. file, directory, etc.)
 * @return the lock of a mounted SD card or otherwise null
 */
std::shared_ptr<Lock> findSdCardLock(const std::string& path);

} // namespace tt::hal
