#pragma once

#include <Tactility/Lock.h>

#include <memory>

namespace tt::lvgl {

std::shared_ptr<Lock> getSyncLock() __attribute__((deprecated("Use lvgl locking functions from lvgl-module")));

} // namespace
