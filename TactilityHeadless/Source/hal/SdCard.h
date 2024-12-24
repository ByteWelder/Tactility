#pragma once

#include "TactilityCore.h"

namespace tt::hal {

#define TT_SDCARD_MOUNT_POINT "/sdcard"

class SdCard {
public:
    enum State {
        StateMounted,
        StateUnmounted,
        StateError,
        StateUnknown
    };

    enum MountBehaviour {
        MountBehaviourAtBoot, /** Only mount at boot */
        MountBehaviourAnytime /** Mount/dismount any time */
    };

private:
    MountBehaviour mountBehaviour;

public:
    explicit SdCard(MountBehaviour mountBehaviour) : mountBehaviour(mountBehaviour) {}
    virtual ~SdCard() = default;

    virtual bool mount(const char* mountPath) = 0;
    virtual bool unmount() = 0;
    virtual State getState() const = 0;

    virtual MountBehaviour getMountBehaviour() const { return mountBehaviour; }
    bool isMounted() const { return getState() == StateMounted; }
};

} // namespace
