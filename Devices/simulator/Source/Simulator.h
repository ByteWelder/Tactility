#pragma once

#include "Main.h"

namespace simulator {
    /** Set the function pointer of the real app_main() */
    void setMain(MainFunction mainFunction);
    /** The actual main task */
    void freertosMain();
}

extern "C" {
void app_main(); // ESP-IDF's main function, implemented in the application
}

int main() {
    // Actual main function that passes on app_main() (to be executed in a FreeRTOS task) and bootstraps FreeRTOS
    simulator::setMain(app_main);
    simulator::freertosMain();
    return 0;
}
