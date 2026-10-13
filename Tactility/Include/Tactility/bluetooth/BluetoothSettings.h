#pragma once

namespace tt::bluetooth::settings {

void setEnable(bool enable);
bool isEnabled();

void setSppAutoStart(bool enable);
bool shouldSppAutoStart();

void setMidiAutoStart(bool enable);
bool shouldMidiAutoStart();

} // namespace tt::bluetooth::settings
