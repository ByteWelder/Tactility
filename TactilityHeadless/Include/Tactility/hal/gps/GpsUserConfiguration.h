#pragma once

namespace tt::hal::gps {

struct GpsUserConfiguration {
    char name[32];
    char uartName[32]; // e.g. "Internal" or "/dev/ttyUSB0"
    uint32_t baudRate;
    GpsModel model; // Choosing "Unknown" will result in a probe
};

}
