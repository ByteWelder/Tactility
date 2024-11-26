#pragma once

#include "hal/Configuration.h"

extern const tt::hal::Configuration sim_hardware;

typedef void (*MainFunction)();
void setMainForSim(MainFunction mainFunction);
void executeMainFunction();
int main_stub();
