#pragma once

#include <cstdint>

namespace tt::settings::keyboard {

struct KeyboardSettings {
    bool backlightEnabled;
    uint8_t backlightBrightness; // 0-255
    bool trackballEnabled;
    bool backlightTimeoutEnabled;
    uint32_t backlightTimeoutMs; // Timeout in milliseconds
};

bool load(KeyboardSettings& settings);

KeyboardSettings loadOrGetDefault();

KeyboardSettings getDefault();

bool save(const KeyboardSettings& settings);

}
