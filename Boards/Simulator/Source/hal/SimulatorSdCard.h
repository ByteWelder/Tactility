#pragma once

#include <Tactility/hal/sdcard/SdCard.h>
#include <Tactility/Mutex.h>
#include <memory>

using tt::hal::sdcard::SdCard;

class SimulatorSdCard final : public SdCard {

private:

    State state;
    std::shared_ptr<tt::Lockable> lockable;
    std::string mountPath;

public:

    SimulatorSdCard() : SdCard(MountBehaviour::AtBoot),
        state(State::Unmounted),
        lockable(std::make_shared<tt::Mutex>())
    {}

    std::string getName() const final { return "Mock SD Card"; }
    std::string getDescription() const final { return ""; }

    bool mount(const std::string& newMountPath) final {
        state = State::Mounted;
        mountPath = newMountPath;
        return true;
    }

    bool unmount() override {
        state = State::Unmounted;
        mountPath = "";
        return true;
    }

    std::string getMountPath() const final { return mountPath; };

    std::shared_ptr<tt::Lockable> getLockable() const final { return lockable; }

    State getState() const override { return state; }
};

