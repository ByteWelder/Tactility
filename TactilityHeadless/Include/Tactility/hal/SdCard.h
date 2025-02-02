#pragma once

#include "Device.h"

#include <Tactility/TactilityCore.h>

namespace tt::hal {

#define TT_SDCARD_MOUNT_NAME "sdcard"
#define TT_SDCARD_MOUNT_POINT "/sdcard"

class SdCard : public Device {

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

    explicit SdCard(MountBehaviour mountBehaviour) : mountBehaviour(mountBehaviour) {}
    virtual ~SdCard() override = default;

    Type getType() const final { return Type::SdCard; };

    virtual bool mount(const char* mountPath) = 0;
    virtual bool unmount() = 0;
    virtual State getState() const = 0;

    virtual MountBehaviour getMountBehaviour() const { return mountBehaviour; }
    bool isMounted() const { return getState() == State::Mounted; }
};

} // namespace
