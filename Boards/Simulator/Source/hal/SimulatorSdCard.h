#pragma once

#include "hal/SdCard.h"

using namespace tt::hal;

class SimulatorSdCard : public SdCard {
private:
    State state;
public:
    SimulatorSdCard() : SdCard(MountBehaviour::AtBoot), state(State::Unmounted) {}

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

