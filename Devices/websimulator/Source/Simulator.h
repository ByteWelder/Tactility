#pragma once

#include "Main.h"

#include <sys/stat.h>
#include <thread>

namespace websimulator {
    /** Set the function pointer of the real app_main() */
    void setMain(MainFunction mainFunction);
    /** The actual main task */
    void freertosMain();
}

extern "C" {
void app_main(); // implemented in Tactility/Source/Main.cpp
}

int main() {
    // The folder for the "data" file system is not in the repository; Emscripten's MEMFS
    // starts empty, so the platform's partitions.cpp realpath("data") would fail without it.
    mkdir("data", 0755);

    // Emscripten's SDL video driver proxies all canvas work to the browser main thread itself,
    // so unlike the desktop simulator there is no separate SDL loop thread: the whole runtime
    // (main() here, and FreeRTOS below) runs on a pthread via -sPROXY_TO_PTHREAD, and the
    // browser main thread only hosts SDL's proxied canvas calls and event delivery.
    websimulator::setMain(app_main);
    websimulator::freertosMain();
    return 0;
}
