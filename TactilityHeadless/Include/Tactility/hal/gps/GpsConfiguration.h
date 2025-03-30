#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace tt::hal::gps {

enum class GpsModel {
    Unknown = 0,
    AG3335,
    AG3352,
    ATGM336H, // Casic (might work with AT6558, Neoway N58 LTE Cat.1, Neoway G2, Neoway G7A)
    LS20031,
    MTK,
    MTK_L76B,
    MTK_PA1616S,
    UBLOX6,
    UBLOX7,
    UBLOX8,
    UBLOX9,
    UBLOX10,
    UC6580,
};

const char* toString(GpsModel model);

std::vector<std::string> getModels();

struct GpsConfiguration {
    char uartName[32]; // e.g. "Internal" or "/dev/ttyUSB0"
    uint32_t baudRate;
    GpsModel model; // Choosing "Unknown" will result in a probe
};

}
