#pragma once

#include <Tactility/Lockable.h>

#include <memory>

namespace tt::lvgl {

/**
 * LVGL locking function
 * @param[in] timeoutMillis timeout in milliseconds. waits forever when 0 is passed.
 * @warning this works with milliseconds, as opposed to every other FreeRTOS function that works in ticks!
 * @warning when passing zero, we wait forever, as this is the default behaviour for esp_lvgl_port, and we want it to remain consistent
 */
typedef bool (*LvglLock)(uint32_t timeoutMillis);
typedef void (*LvglUnlock)();

void syncSet(LvglLock lock, LvglUnlock unlock);

/**
 * LVGL locking function
 * @param[in] timeout as ticks
 * @warning when passing zero, we wait forever, as this is the default behaviour for esp_lvgl_port, and we want it to remain consistent
 */
bool lock(TickType_t timeout);
void unlock();

std::shared_ptr<Lockable> getLvglSyncLockable();

} // namespace
