#pragma once

namespace tt::lvgl {

#ifdef ESP_PLATFORM
static constexpr auto* PATH_PREFIX = "A:";
#else
// PC paths are relative, unlike ESP paths.
// LVGL paths require a `/` prefix, so we have to add it here:
static constexpr auto* PATH_PREFIX = "A:/";
#endif

bool isStarted();

void start();

void stop();

}
