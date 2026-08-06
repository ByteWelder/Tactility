#pragma once

struct Device;

// Constructs, parents, binds and starts the ST7123 in-cell touch device on i2c0. Shared by the V2
// (ST7123 display) and V3 (ST7121 display) variants - both boards use the exact same touch
// controller/address/wiring, only the display panel differs. Not used by V1 (ILI9881C + GT911).
void create_st7123_touch(Device* i2c0);
