#include "Simulator.h"

MainFunction mainFunction = nullptr;

void setMainForSim(MainFunction newMainFunction) {
    mainFunction = newMainFunction;
}

void executeMainFunction() {
    assert(mainFunction);
    mainFunction();
}
