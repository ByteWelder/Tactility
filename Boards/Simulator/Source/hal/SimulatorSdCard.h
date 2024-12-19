#pragma once

#include "hal/SdCard.h"

using namespace tt::hal;

class SimulatorSdCard : public SdCard {
private:
    State state;
public:
    SimulatorSdCard() : SdCard(MountBehaviourAtBoot), state(StateUnmounted) {}

    bool mount(const char* mountPath) override {
        state = StateMounted;
        return true;
    }
    bool unmount() override {
        state = StateUnmounted;
        return true;
    }

    State getState() const override {
        return state;
    }
};

