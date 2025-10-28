#pragma once

#include "Tactility/hal/sdcard/SdCardDevice.h"
#include <Tactility/Mutex.h>
#include <memory>

using tt::hal::sdcard::SdCardDevice;

class SimulatorSdCard final : public SdCardDevice {

    State state;
    std::shared_ptr<tt::Lock> lock;
    std::string mountPath;

public:

    SimulatorSdCard() : SdCardDevice(MountBehaviour::AtBoot),
        state(State::Unmounted),
        lock(std::make_shared<tt::Mutex>(tt::Mutex::Type::Recursive))
    {}

    std::string getName() const override { return "Mock SD Card"; }
    std::string getDescription() const override { return ""; }

    bool mount(const std::string& newMountPath) override {
        state = State::Mounted;
        mountPath = newMountPath;
        return true;
    }

    bool unmount() override {
        state = State::Unmounted;
        mountPath = "";
        return true;
    }

    std::string getMountPath() const override { return mountPath; }

    std::shared_ptr<tt::Lock> getLock() const override { return lock; }

    State getState(TickType_t timeout) const override { return state; }
};
