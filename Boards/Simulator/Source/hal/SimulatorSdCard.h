#pragma once

#include <Tactility/hal/SdCard.h>

using namespace tt::hal;

class SimulatorSdCard final : public SdCard {

private:

    State state;

public:

    SimulatorSdCard() : SdCard(MountBehaviour::AtBoot), state(State::Unmounted) {}

    std::string getName() const final { return "Mock SD Card"; }
    std::string getDescription() const final { return ""; }

    bool mount(const char* mountPath) override {
        state = State::Mounted;
        return true;
    }

    bool unmount() override {
        state = State::Unmounted;
        return true;
    }

    State getState() const override {
        return state;
    }
};

